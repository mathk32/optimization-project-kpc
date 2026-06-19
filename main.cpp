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