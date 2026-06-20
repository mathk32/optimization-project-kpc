#include <iostream>
#include <vector>
#include <fstream> 
#include <filesystem>
#include <ilcplex/ilocplex.h>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

struct Item {
    int id;
    double profit;
    double weight;
    int degree;
};

struct KPCInstance{
    int num_items;
    double capacity;
    std::vector<Item> items;

    std::vector<std::vector<bool>> conflict_matrix;
};

struct ExperimentKPC{
    double max_profit; 
    int explored_nodes;
    double cpu_time; 
};

KPCInstance read_instance(const std::string& file_name){
    std::ifstream file(file_name);
    KPCInstance inst;

    if(!file.is_open()){
        std::cerr << "Error reading file." << file_name << std::endl;
        exit(1);
    }

    file >> inst.num_items >> inst.capacity;
    inst.conflict_matrix.assign(
        inst.num_items, 
        std::vector<bool>(inst.num_items, false)
    );

    for(int i = 0; i < inst.num_items; ++i){
        Item item;
        item.id = i;
        file >> item.profit >> item.weight;
        item.degree = 0;
        inst.items.push_back(item);
    }

    int num_edges;
    file >> num_edges;

    for(int i=0; i < num_edges; ++i){
        int u, v;
        file >> u >> v;
        inst.conflict_matrix[u][v] = true;
        inst.conflict_matrix[v][u] = true;

        inst.items[u].degree++;
        inst.items[v].degree++;
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
            
            double efficiency = inst.items[i].profit / inst.items[i].weight;
            double score = inst.items[i].degree * efficiency;

      
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

ExperimentKPC solveKPC(const KPCInstance inst, bool use_heuristics){
    IloEnv env;
    ExperimentKPC res = {0.0,0,0.0};

    try{

        IloModel model(env);
        IloNumVarArray x(env, inst.num_items, 0,1, ILOINT);

        IloExpr objExpr(env);
        for (int i =0; i < inst.num_items; ++i){
            objExpr += inst.items[i].profit*x[i];
        }
        model.add(IloMaximize(env, objExpr));
        objExpr.end();

        IloExpr weightExpr(env);
        for(int i=0; i < inst.num_items; ++i){
            weightExpr += inst.items[i].weight *x[i];
        }
        model.add(weightExpr <= inst.capacity);
        weightExpr.end();

        for(int i=0; i < inst.num_items; ++i){
            for(int j = i + 1; j < inst.num_items; ++j){
                if(inst.conflict_matrix[i][j]){
                    model.add(x[i] + x[j] <= 1);
                }
            }
        }

        IloCplex cplex(model);
        cplex.setOut(env.getNullStream());
        
        if(use_heuristics){
            cplex.use(MyBranchRule(env, x, inst));
            cplex.setParam(IloCplex::Param::MIP::Strategy::FPHeur, -1);
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        
        cplex.solve();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;

        if (cplex.getStatus() == IloAlgorithm::Optimal){
            res.max_profit = cplex.getObjValue();
            res.explored_nodes = cplex.getNnodes();
            res.cpu_time = diff.count();
        }

    }catch(IloException e){
        std::cerr << "CPLEX error: " << e << std::endl;
    }

    env.end();
    return res;


};

int main(){

    std::string instance_folder = "../instances";
    std::ofstream csv_file("results.csv");

    csv_file << "Instance, Items, Default_Profit, Default_Nodes, Default_Time(s), Profit_Score, Score_Nodes, Score_Time(s)\n";

    for(const auto& entry: fs::directory_iterator(instance_folder)){
        std::string path = entry.path().string();
        std::string file_name = entry.path().filename().string();
            
        std::cout << "Processing: " << file_name << "..." << std::flush;

        KPCInstance inst = read_instance(path);

        ExperimentKPC res_default = solveKPC(inst, false);
            
        ExperimentKPC res_score = solveKPC(inst, true);

        csv_file << file_name << ","
                    << inst.num_items << ","
                    << res_default.max_profit<< ","
                    << res_default.explored_nodes << ","
                    << std::fixed << std::setprecision(4) << res_default.cpu_time << ","
                    << res_score.max_profit << ","
                    << res_score.explored_nodes << ","
                    << std::fixed << std::setprecision(4) << res_score.cpu_time << "\n";
                        
        std::cout << " OK!" << std::endl;
    }

    csv_file.close();
    std::cout << "\nTest completed! 'results.csv' file generated successfully." << std::endl;

    return 0;
}