import os
import random 


def generate_instances(file_name , num_items, density):
    weights = [random.randint(10,100) for _ in range(num_items)]
    profits = [random.randint(10,100) for _ in range(num_items)]

    capacity = sum(weights) // 2
    edges = []

    for i in range(num_items):
        for j in range(i + 1, num_items):
            if random.random() < density:
                edges.append((i,j))

    with open(file_name, 'w') as f:
        f.write(f"{num_items} {capacity}\n")
        for p,w in zip (profits, weights):
            f.write(f"{p} {w}\n")
        
        f.write(f"{len(edges)}\n")
        for u,v in edges:
            f.write(f"{u} {v}\n")

if __name__ == "__main__":

    os.makedirs("instances", exist_ok=True)
        
    settings = [(50, 0.2, 5),  
            (50, 0.5, 5),
            (100, 0.2, 5), 
            (100, 0.8, 5)]
        
    count = 1
    for num_items, density, amt in settings:
        for _ in range(amt):
            name = f"instances/inst_{count:02d}_n{num_items}_d{int(density*100)}.txt"
            generate_instances(name, num_items, density)
            count += 1
                
    print("20 instances successfully generated.!")    
            
