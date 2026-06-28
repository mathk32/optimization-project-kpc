#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <ilcplex/ilocplex.h>
#include <chrono>
#include <iomanip>
#include <cmath>

namespace fs = std::filesystem;

struct Item {
    int id;
    double profit;
    double weight;
    int degree;
    double branch_score;
};

struct KPCInstance{
    int num_items;
    double capacity;
    double density;
    std::vector<Item> items;
    std::vector<std::vector<bool>> conflict_matrix;
};

struct ExperimentKPC{
    double max_profit;
    long long explored_nodes;
    double cpu_time;
    bool is_optimal;
};

KPCInstance read_instance(const std::string& file_name){
    std::ifstream file(file_name);
    KPCInstance inst;

    if(!file.is_open()){
        std::cerr << "Error reading file: " << file_name << std::endl;
        exit(1);
    }

    file >> inst.num_items >> inst.capacity;
    inst.conflict_matrix.assign(inst.num_items, std::vector<bool>(inst.num_items, false));

    for(int i = 0; i < inst.num_items; ++i){
        Item item;
        item.id = i;
        file >> item.profit >> item.weight;
        item.degree = 0;
        item.branch_score = 0.0;
        inst.items.push_back(item);
    }

    int num_edges;
    file >> num_edges;

    for(int i=0; i < num_edges; ++i){
        int u, v;
        file >> u >> v;

        if(u < 0 || u >= inst.num_items || v < 0 || v >= inst.num_items){
            std::cerr << "Error: invalid edge (" << u << ", " << v
                       << ") in file " << file_name << std::endl;
            exit(1);
        }

        inst.conflict_matrix[u][v] = true;
        inst.conflict_matrix[v][u] = true;
        inst.items[u].degree++;
        inst.items[v].degree++;
    }

    if (inst.num_items > 1) {
        inst.density = (2.0 * num_edges) / (static_cast<double>(inst.num_items) * (inst.num_items - 1));
    } else {
        inst.density = 0.0;
    }

    int max_degree = 0;
    double max_profit = 0.0;
    double max_weight = 0.0;

    for(int i = 0; i < inst.num_items; ++i){
        if(inst.items[i].degree > max_degree) max_degree = inst.items[i].degree;
        if(inst.items[i].profit > max_profit) max_profit = inst.items[i].profit;
        if(inst.items[i].weight > max_weight) max_weight = inst.items[i].weight;
    }

    if(max_degree == 0) max_degree = 1;
    if(max_profit == 0.0) max_profit = 1.0;
    if(max_weight == 0.0) max_weight = 1.0;

    double exponent = std::max(0.5, 1.0 - inst.density);

    for(int i = 0; i < inst.num_items; ++i){
        double normalized_degree = static_cast<double>(inst.items[i].degree) / max_degree;
        double normalized_profit = inst.items[i].profit / max_profit;
        double normalized_weight = inst.items[i].weight / max_weight;

        inst.items[i].branch_score = (1.0 + normalized_degree) * (normalized_profit / std::pow(normalized_weight, exponent));
    }

    file.close();
    return inst;
};


ILOBRANCHCALLBACK2(MyBranchRule, IloNumVarArray, x, const KPCInstance&, inst) {
    IloEnv env = getEnv();
    IloNumArray val(env);
    getValues(val, x);

    int best_var = -1;
    double max_score = -1.0;
    double min_dist_to_half = 1.0;

    for (int i = 0; i < inst.num_items; ++i) {
        if (val[i] > 1e-5 && val[i] < 1.0 - 1e-5) {
            double score = inst.items[i].branch_score;
            double dist = std::abs(0.5 - val[i]);

            if (score > max_score || (score == max_score && dist < min_dist_to_half)) {
                best_var = i;
                max_score = score;
                min_dist_to_half = dist;
            }
        }
    }

    if (best_var != -1) {
        double obj_val = getObjValue();
        makeBranch(x[best_var], 1.0, IloCplex::BranchUp, obj_val);
        makeBranch(x[best_var], 0.0, IloCplex::BranchDown, obj_val);
    }
    val.end();
}

ILOBRANCHCALLBACK2(MostFractionalRule, IloNumVarArray, x, const KPCInstance&, inst) {
    IloEnv env = getEnv();
    IloNumArray val(env);
    getValues(val, x);

    int best_var = -1;
    double min_dist_to_half = 1.0;

    for (int i = 0; i < inst.num_items; ++i) {
        if (val[i] > 1e-5 && val[i] < 1.0 - 1e-5) {
            double dist = std::abs(0.5 - val[i]);
            if (dist < min_dist_to_half) {
                best_var = i;
                min_dist_to_half = dist;
            }
        }
    }

    if (best_var != -1) {
        double obj_val = getObjValue();
        makeBranch(x[best_var], 1.0, IloCplex::BranchUp, obj_val);
        makeBranch(x[best_var], 0.0, IloCplex::BranchDown, obj_val);
    }
    val.end();
}

enum class BranchMode {
    OUR_SCORE,
    MOST_FRACTIONAL,
    CPLEX_DEFAULT
};

ExperimentKPC solveKPC(const KPCInstance& inst, BranchMode mode, bool disable_cplex_features){
    IloEnv env;
    ExperimentKPC res = {0.0, 0, 0.0, false};

    try{
        IloModel model(env);
        IloNumVarArray x(env, inst.num_items, 0, 1, ILOINT);

        IloExpr objExpr(env);
        for (int i = 0; i < inst.num_items; ++i){
            objExpr += inst.items[i].profit * x[i];
        }
        model.add(IloMaximize(env, objExpr));
        objExpr.end();

        IloExpr weightExpr(env);
        for(int i = 0; i < inst.num_items; ++i){
            weightExpr += inst.items[i].weight * x[i];
        }
        model.add(weightExpr <= inst.capacity);
        weightExpr.end();

        for(int i = 0; i < inst.num_items; ++i){
            for(int j = i + 1; j < inst.num_items; ++j){
                if(inst.conflict_matrix[i][j]){
                    model.add(x[i] + x[j] <= 1);
                }
            }
        }

        IloCplex cplex(model);
        cplex.setOut(env.getNullStream());

        if(disable_cplex_features){
            cplex.setParam(IloCplex::Param::MIP::Strategy::FPHeur, -1);
            cplex.setParam(IloCplex::Param::Preprocessing::Presolve, IloFalse);
            cplex.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq, -1);
            cplex.setParam(IloCplex::Param::MIP::Limits::CutsFactor, 1.0);
        }

        if(mode == BranchMode::OUR_SCORE){
            cplex.use(MyBranchRule(env, x, inst));
        } else if(mode == BranchMode::MOST_FRACTIONAL){
            cplex.use(MostFractionalRule(env, x, inst));
        }
  
        auto start_time = std::chrono::high_resolution_clock::now();
        cplex.solve();
        auto end_time = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = end_time - start_time;
        res.cpu_time = diff.count();
        res.explored_nodes = static_cast<long long>(cplex.getNnodes64());
        res.is_optimal = (cplex.getStatus() == IloAlgorithm::Optimal);

        if (res.is_optimal){
            res.max_profit = cplex.getObjValue();
        }

    }catch(const IloException& e){
        std::cerr << "CPLEX error: " << e << std::endl;
    }

    env.end();
    return res;
};

int main(){
    std::string instance_folder = "../instances";

    std::ofstream csv_pre("pre_solver_results.csv");
    std::ofstream csv_post("post_solver_results.csv");

    std::string header = "Instance,Items,Density,Default_Profit,Default_Nodes,Default_Time(s),Default_Optimal,"
                          "Score_Profit,Score_Nodes,Score_Time(s),Score_Optimal\n";

    csv_pre << header;
    csv_post << header;

    for(const auto& entry: fs::directory_iterator(instance_folder)){
        std::string path = entry.path().string();
        std::string file_name = entry.path().filename().string();

        std::cout << "Processing: " << file_name << "..." << std::flush;

        KPCInstance inst = read_instance(path);

        ExperimentKPC res_pre_default = solveKPC(inst, BranchMode::MOST_FRACTIONAL, true);
        ExperimentKPC res_pre_score   = solveKPC(inst, BranchMode::OUR_SCORE, true);

        ExperimentKPC res_post_default = solveKPC(inst, BranchMode::CPLEX_DEFAULT, false);
        ExperimentKPC res_post_score   = solveKPC(inst, BranchMode::OUR_SCORE, false);

        csv_pre << file_name << ","
                << inst.num_items << ","
                << std::fixed << std::setprecision(2) << inst.density << ","
                << std::fixed << std::setprecision(4) << res_pre_default.max_profit << ","
                << res_pre_default.explored_nodes << ","
                << std::fixed << std::setprecision(4) << res_pre_default.cpu_time << ","
                << (res_pre_default.is_optimal ? "1" : "0") << ","
                << std::fixed << std::setprecision(4) << res_pre_score.max_profit << ","
                << res_pre_score.explored_nodes << ","
                << std::fixed << std::setprecision(4) << res_pre_score.cpu_time << ","
                << (res_pre_score.is_optimal ? "1" : "0") << "\n";

        csv_post << file_name << ","
                 << inst.num_items << ","
                 << std::fixed << std::setprecision(2) << inst.density << ","
                 << std::fixed << std::setprecision(4) << res_post_default.max_profit << ","
                 << res_post_default.explored_nodes << ","
                 << std::fixed << std::setprecision(4) << res_post_default.cpu_time << ","
                 << (res_post_default.is_optimal ? "1" : "0") << ","
                 << std::fixed << std::setprecision(4) << res_post_score.max_profit << ","
                 << res_post_score.explored_nodes << ","
                 << std::fixed << std::setprecision(4) << res_post_score.cpu_time << ","
                 << (res_post_score.is_optimal ? "1" : "0") << "\n";

        std::cout << " OK!" << std::endl;
    }

    csv_pre.close();
    csv_post.close();

    std::cout << "\nTests completed! Files 'pre_solver_results.csv' and 'post_solver_results.csv' successfully generated." << std::endl;

    return 0;
}