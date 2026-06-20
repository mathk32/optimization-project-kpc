#include <iostream>
#include <vector>
#include <fstream> 
#include <ilcplex/ilocplex.h>

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



void solveKPC(const KPCInstance inst){
    IloEnv env;
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
        
        cplex.use(MyBranchRule(env, x, inst));
        cplex.setParam(IloCplex::Param::MIP::Strategy::FPHeur, -1);

        if(cplex.solve()){
            std::cout << "----------------------------------------------------" << std::endl;
            std::cout << "Status: " << cplex.getStatus() << std::endl;
            std::cout << "Maximum Profit: "<< cplex.getObjValue() << std::endl;

            std::cout << "Selected Items: ";
            for(int i=0; i < inst.num_items; ++i){
                if(cplex.getValue(x[i]) > 0.5){
                    std::cout << i << " ";
                }
            }
            std::cout << std::endl;
        }else{
            std::cout << " No Solution Found." << std::endl;
        }
    }catch(IloException e){
        std::cerr << "CPLEX error: " << e << std::endl;
    }

    env.end();


};

int main(){

    std::cout << "Data Structures loaded successfully." << std::endl;

    KPCInstance instance = read_instance("instance.txt");

    solveKPC(instance);

    return 0;
}