/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

//#define VIENNACL_DEBUG_BUILD
//#define VIENNACL_WITH_OPENCL
//#define VIENNACL_DEBUG_ALL

#include <iostream>

#include "viennacl/linalg/prod.hpp"
#include "viennacl/vector.hpp"
#include "viennacl/matrix.hpp"
#include "boost/numeric/ublas/matrix.hpp"
#include "viennacl/generator/generate.hpp"
#include "viennacl/generator/autotune.hpp"

#include "command-line-utils.hpp"

//#define N_RUNS 5

using namespace viennacl::generator;

typedef std::vector< viennacl::ocl::platform > platforms_type;
typedef std::vector<viennacl::ocl::device> devices_type;
typedef std::vector<cl_device_id> cl_devices_type;

static const unsigned int size = 2048;

struct autotuner_options{

    std::string layout;
    std::string scalartype;
    std::string output_name;

    unsigned int requested_device;

    std::string vector_interval;

    std::string local_size_1_interval;
    std::string local_size_2_interval;
    std::string num_groups_interval;
};

autotuner_options get_options(int argc, char* argv[]){
    try{
        autotuner_options options;

        TCLAP::CmdLine cmd("GEMM Autotuner", ' ', "0.1");


        pow_2_interval_constraint pow_2_interval_cstrt;
        min_max_inc_constraint min_max_inc_cstrt;

        //Layouts
        std::vector<std::string> allowed_layouts;
        allowed_layouts.push_back("Nx");
        allowed_layouts.push_back("Tx");
        TCLAP::ValuesConstraint<std::string> allowed_layouts_constraint( allowed_layouts);
        TCLAP::ValueArg<std::string> layout_arg("l","layout","Layout to tune the hardware for",true,"Nx",&allowed_layouts_constraint,cmd);

        //Scalartype
        std::vector<std::string> allowed_scalartypes;
        allowed_scalartypes.push_back("float");
        allowed_scalartypes.push_back("double");
        TCLAP::ValuesConstraint<std::string> allowed_scalartypes_constraint( allowed_scalartypes);
        TCLAP::ValueArg<std::string> scalartype_arg("s","scalartype","Scalartype to tune the hardware for",true,"float",&allowed_scalartypes_constraint,cmd);

        //Output data file
        TCLAP::ValueArg<std::string> output_name_arg("o","output","Name of the output data file",true,"gemm_autotuning.dat","string",cmd);

        //Device id
        TCLAP::ValueArg<unsigned int> requested_device_arg("d","device","ID of the device to use for the autotuning procedure",false,0,"unsigned int",cmd);

        //Vector
        TCLAP::ValueArg<std::string> vector_interval_arg("","vector","Vector type used in the kernel",false,"1,1",&pow_2_interval_cstrt,cmd);

        //Large blocks
        TCLAP::ValueArg<std::string> local_size_1_interval_arg("","local-size-1","Number of work-item rows in each work-group. Specify min,max both power of two.",false,"2,64",&pow_2_interval_cstrt,cmd);
        TCLAP::ValueArg<std::string> local_size_2_interval_arg("","local-size-2","Number of work-item columns in each work-group. Specify min,max both power of two.",false,"2,64",&pow_2_interval_cstrt,cmd);
        TCLAP::ValueArg<std::string> num_groups_interval_arg("","num-groups","Number of work groups required.",false,"1,1024,16",&min_max_inc_cstrt,cmd);


        cmd.parse(argc,argv);
        options.layout = layout_arg.getValue();
        options.scalartype = scalartype_arg.getValue();
        options.output_name = output_name_arg.getValue();
        options.requested_device = requested_device_arg.getValue();
        options.vector_interval = vector_interval_arg.getValue();
        options.local_size_1_interval = local_size_1_interval_arg.getValue();
        options.local_size_2_interval = local_size_2_interval_arg.getValue();
        options.num_groups_interval = num_groups_interval_arg.getValue();
        return options;
    }
    catch (TCLAP::ArgException &e){
        std::cerr << "error: " << "\"" << e.error() << "\"" << " [for arg " << e.argId() << "]" << std::endl;
        exit(EXIT_FAILURE);
    }
}

template<class ScalarType>
struct blas2_config{
    typedef vector_reduction profile_type;
    static profile_type create_profile(std::map<std::string, autotune::tuning_param> const & params){
      return profile_type(params.at("vector").current(), params.at("local_size1").current(),params.at("local_size2").current(),params.at("num_groups").current());
    }
    static bool is_invalid(viennacl::ocl::device const & dev, std::map<std::string, autotune::tuning_param> const & params){
        profile_type prof = create_profile(params);
        return prof.is_invalid(dev, sizeof(ScalarType));
    }
};

template<class ScalarType>
code_generator::forced_profile_key_type make_key(autotuner_options options){
    if(options.layout=="Nx") return code_generator::forced_profile_key_type(VECTOR_REDUCE_Nx_TYPE,sizeof(ScalarType));
    else return code_generator::forced_profile_key_type(VECTOR_REDUCE_Tx_TYPE,sizeof(ScalarType));;
}

template<class ScalarType>
viennacl::scheduler::statement make_statement(autotuner_options options, viennacl::vector<ScalarType> const & y, viennacl::matrix<ScalarType> const & A, viennacl::vector<ScalarType> const & x){
    if(options.layout =="Nx") return viennacl::scheduler::statement(y,viennacl::op_assign(), viennacl::linalg::prod(A, x));
    else return viennacl::scheduler::statement(y,viennacl::op_assign(), viennacl::linalg::prod(viennacl::trans(A), x));
}

template<class ScalarType>
void run_autotune(autotuner_options const & options, viennacl::ocl::device const & device){
    viennacl::vector<ScalarType> y(size), x(size);
    viennacl::matrix<ScalarType> A(size,size);
    std::map<double,typename blas2_config<ScalarType>::profile_type> timings;
    autotune::tuning_config< blas2_config<ScalarType> > conf;

    std::vector<unsigned int> tmp;
    tmp = get_values_in_comas(options.vector_interval); std::vector<int> vector; for(unsigned int i = tmp[0] ; i <= tmp[1] ; i*=2) vector.push_back(i);
    tmp = get_values_in_comas(options.local_size_1_interval); std::vector<int> local_size_1; for(unsigned int i = tmp[0] ; i <= tmp[1] ; i*=2) local_size_1.push_back(i);
    tmp = get_values_in_comas(options.local_size_2_interval); std::vector<int> local_size_2; for(unsigned int i = tmp[0] ; i <= tmp[1] ; i*=2) local_size_2.push_back(i);
    tmp = get_values_in_comas(options.num_groups_interval); std::vector<int> num_groups; for(unsigned int i = tmp[0] ; i <= tmp[1] ; i+=tmp[2]) num_groups.push_back(i);

    conf.add_tuning_param("vector",vector);
    conf.add_tuning_param("local_size1",local_size_1);
    conf.add_tuning_param("local_size2",local_size_2);
    conf.add_tuning_param("num_groups",num_groups);
    std::ofstream stream(options.output_name.c_str());
    code_generator::forced_profile_key_type key = make_key<ScalarType>(options);
    autotune::benchmark(&timings,make_statement(options,y,A,x),key,conf,&stream);
    std::cout << std::endl;
    std::cout << " ============" << std::endl;
    std::cout << " Best Profile : " << std::scientific << timings.begin()->first << " => " << timings.begin()->second << std::endl;
    std::cout << " ============" << std::endl;
    std::cout << std::endl;
}


int main(int argc, char* argv[]){
  typedef std::vector< viennacl::ocl::platform > platforms_type;
  typedef std::vector<viennacl::ocl::device> devices_type;
  unsigned int counter = 0;
  autotuner_options options = get_options(argc,argv);
  platforms_type platforms = viennacl::ocl::get_platforms();
  for (platforms_type::iterator platform_iter  = platforms.begin();
       platform_iter != platforms.end();
       ++platform_iter)
  {
    devices_type devices = platform_iter->devices(CL_DEVICE_TYPE_ALL);
    for(devices_type::iterator iter = devices.begin(); iter != devices.end(); iter++)
    {
      if(counter++==options.requested_device){
        viennacl::ocl::setup_context(counter,*iter);
        viennacl::ocl::switch_context(counter);
        viennacl::ocl::device const & device = viennacl::ocl::current_device();
        std::string device_name = device.name();
        std::transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
        std::replace(device_name.begin(), device_name.end(),' ', '_');
        std::cout << "-------------------" << std::endl;
        std::cout << device.info() << std::endl;
        std::cout << "Operation : GEMV" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::cout << "layout : " << options.layout << std::endl;
        std::cout << "scalatype : " << options.scalartype << std::endl;
        std::cout << "vector : [" << options.vector_interval << "]" << std::endl;
        std::cout << "local size 1 : [" << options.local_size_1_interval << "]" << std::endl;
        std::cout << "local size 2 : [" << options.local_size_2_interval << "]" << std::endl;
        std::cout << "number of groups : [" << options.num_groups_interval << "]" << std::endl;
        std::cout << "-------------------" << std::endl;
        if(options.scalartype=="float")
            run_autotune<float>(options,device);
        else if(options.scalartype=="double")
            run_autotune<double>(options,device);

      }
    }
  }
}
