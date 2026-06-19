#include <iostream>
#include <vector>
#include <fstream> 

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

int main(){

    std::cout << "Data Structures loaded successfully." << std::endl;

    KPCInstance instance = read_instance("instance.txt");

    std::cout << "File read successfully." << std::endl;
    std::cout << "We have: " << instance.num_items << " items and capacity " << instance.capacity << std::endl;
    std::cout << "Item 0 has " << instance.items[0].degree << " conflict(s)." << std::endl;


    return 0;
}