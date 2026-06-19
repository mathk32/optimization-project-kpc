#include <iostream>
#include <vector>

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

int main(){
    std::cout << "Data Structures loaded successfully." << std::endl;


    return 0;
}