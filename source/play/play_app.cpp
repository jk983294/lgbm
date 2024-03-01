#include <LightGBM/application.h>

#include <iostream>

using namespace LightGBM;

int main() {
  bool success = false;
  try {
    LightGBM::Application app;
    std::unordered_map<std::string, std::string> params;
    params["task"] = "train";
    params["boosting"] = "gbdt";
    params["data_sample_strategy"] = "bagging";
    params["objective"] = "regression";
    params["metric"] = "l2";
    params["device_type"] = "cpu";
    params["tree_learner"] = "serial";
    params["data"] = "regression.train";
    params["num_trees"] = "100";
    params["learning_rate"] = "0.05";
    params["num_leaves"] = "31";
    params["num_threads"] = "4";
    params["min_data_in_leaf"] = "100";
    params["min_sum_hessian_in_leaf"] = "5.0";
    params["is_save_binary_file"] = "false";
    params["verbose"] = "5";
    app.config_.Set(params);
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
