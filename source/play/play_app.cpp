#include <LightGBM/application.h>

#include <iostream>
#include <string>
#include <unistd.h>

using namespace LightGBM;

static void help() {
    std::cout << "Program options:" << std::endl;
    std::cout << "  -h                                    list help" << std::endl;
    std::cout << "  -m arg (=100000)                          nrow" << std::endl;
    std::cout << "  -n arg (=1000)                          ncol" << std::endl;
    std::cout << "  -p arg (=10000)                          valid_nrow" << std::endl;
    std::cout << "  -t arg (=8)                          thread" << std::endl;
    printf("play_app -m 100000 -p 10000\n");
}

int main(int argc, char** argv) {
  LightGBM::Application app;
  int thread_num = 8;
  std::string config;
  int opt;
  while ((opt = getopt(argc, argv, "hm:n:p:q:t:")) != -1) {
      switch (opt) {
          case 'm':
              app.m_random_nrow = std::stoi(optarg);
              break;
          case 'n':
              app.m_random_ncol = std::stoi(optarg);
              break;
          case 'p':
              app.m_random_valid_nrow = std::stoi(optarg);
              break;
          case 't':
              thread_num = std::stoi(optarg);
              break;
          case 'h':
          default:
              help();
              return 0;
      }
  }

  bool success = false;
  try {
    app.m_is_random_test_data = true;
    std::unordered_map<std::string, std::string> params;
    params["task"] = "train";
    params["boosting"] = "gbdt";
    params["data_sample_strategy"] = "bagging";
    params["objective"] = "regression";
    params["metric"] = "l2";
    params["device_type"] = "cpu";
    params["tree_learner"] = "serial";
    params["num_trees"] = "100";
    params["learning_rate"] = "0.05";
    params["num_leaves"] = "31";
    params["num_threads"] = std::to_string(thread_num);
    params["min_data_in_leaf"] = "100";
    params["min_sum_hessian_in_leaf"] = "5.0";
    params["is_save_binary_file"] = "false";
    params["verbose"] = "5";
    params["output_model"] = "/tmp/LightGBM_model.txt";
    app.config_.Set(params);
    printf("num=%d,%d,%d, thread=%d\n", app.m_random_nrow, app.m_random_ncol, app.m_random_valid_nrow, thread_num);
    OMP_SET_NUM_THREADS(app.config_.num_threads);
    app.check_if_set_device_cuda();

    app.Run();
    success = true;
  }
  catch (const std::exception& ex) {
    std::cerr << "Met Exceptions:" << std::endl;
    std::cerr << ex.what() << std::endl;
  }
  catch (const std::string& ex) {
    std::cerr << "Met Exceptions:" << std::endl;
    std::cerr << ex << std::endl;
  }
  catch (...) {
    std::cerr << "Unknown Exceptions" << std::endl;
  }

  if (!success) {
    exit(-1);
  }
}
