#include <stdint.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <functional>
#include <math.h>
#include "StaticQueue.hpp"
#include "StaticPriorityQueue.hpp"


#define MAX(a,b) ((a > b) ? a : b);
#define MIN(a,b) ((a < b) ? a : b);
#define MAX_HEIGHT 21

template<unsigned ORDER, unsigned DIM, unsigned MAX_POLYGONS, unsigned MAX_VERTEX, unsigned MAX_KNN, bool SFC = false>
class RTree{
public:

    static constexpr unsigned MAX_NODES = MAX_POLYGONS * ORDER;

    static constexpr unsigned POLYGON_ZONE = MAX_NODES;

    static constexpr uint32_t DIRTY = UINT32_MAX;

    using Position = int32_t;
    using Pointer = uint32_t;
    using Size = uint32_t;
    using Identity = uint32_t;
    using Point = std::array<Position, DIM>;

    struct Polygon{
        Identity id;
        Size size = 0;
        Point vertex[MAX_VERTEX];
    };


    struct Box{
        Position mins[DIM];
        Position maxs[DIM];
    };

    struct Node{
        Size size = 0;
        Box box[ORDER];
        Pointer child[ORDER];
    };

private:
    inline static const float log2ORDER = log2(ORDER);
    int size;
    Pointer root;


    Pointer delta[ORDER];
    Pointer parents[MAX_HEIGHT];
    int childs[MAX_HEIGHT];
    Size sizes[MAX_HEIGHT];


    Box obox[ORDER+1];
    Pointer ochild[ORDER+1];
    int oidx[ORDER+1];


    Node nodes[MAX_NODES];
    Polygon polygons[MAX_POLYGONS];

    Size top_dirty_node;
    Pointer dirty_nodes[MAX_NODES];

    Size top_dirty_polygon;
    Pointer dirty_polygons[MAX_POLYGONS];

    Size top_willbe_dirty;
    Pointer willbe_dirty[MAX_POLYGONS];

    struct vBFS{Pointer node_ptr; int lvl;};
    StaticQueue<vBFS, MAX_POLYGONS> bfs;

    struct vKNN{Position distance; Pointer polygon_ptr; Point point; const bool operator<(vKNN const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MAXHEAP, vKNN, MAX_KNN> knn;

    struct vHBF{Position distance; Pointer node_ptr; const bool operator<(vHBF const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MINHEAP, vHBF, MAX_HEIGHT*ORDER + 1> hbf; // H best first



    Node & get_node(Pointer node_ptr){
        return nodes[node_ptr];
    }
    Polygon & get_polygon(Pointer polygon_ptr){
        return polygons[polygon_ptr - POLYGON_ZONE];
    }
    Pointer create_node(){
        return dirty_nodes[top_dirty_node++];
    }
    void destroy_node(Pointer node_ptr){
        dirty_nodes[--top_dirty_node] = node_ptr;
    }
    Pointer create_polygon(Polygon const& polygon){
        polygons[dirty_polygons[top_dirty_polygon]] = polygon;
        return dirty_polygons[top_dirty_polygon++] + POLYGON_ZONE;
    }
    void destroy_polygon(Pointer polygon_ptr){
        get_polygon(polygon_ptr).size = DIRTY;
        dirty_polygons[--top_dirty_polygon] = polygon_ptr - POLYGON_ZONE;
    }
    bool is_not_leaf(Pointer node_ptr){
        return nodes[node_ptr].child[0] < POLYGON_ZONE;
    }
    Box get_mbb(Polygon const& polygon){
        Box box;
        for(int j = 0; j<DIM; ++j) {
            box.mins[j] = polygon.vertex[0][j];
            box.maxs[j] = polygon.vertex[0][j];
        }
        for(int i = 1; i<polygon.size; ++i){
            for(int j = 0; j<DIM; ++j){
                if(box.mins[j] > polygon.vertex[i][j]) box.mins[j] = polygon.vertex[i][j];
                if(box.maxs[j] < polygon.vertex[i][j]) box.maxs[j] = polygon.vertex[i][j];
            }
        }
        return box;
    }

    Position get_delta(Box const& a, Box const& b){
        Position d = 0;
        for(int i = 0; i<DIM; ++i){
            if(a.mins[i] >= b.maxs[i]) d += (a.maxs[i] - b.maxs[i]);
            else if(a.maxs[i] <= b.mins[i]) d += (b.mins[i] - a.mins[i]);
            else{
                if(a.mins[i] < b.mins[i]) d += (b.mins[i] - a.mins[i]);
                if(a.maxs[i] > b.maxs[i]) d += (a.maxs[i] - b.maxs[i]);
            }
        }
        return d;
    }

    int get_best_box(Node& current, Box const& box){
        int best = 0;
        for(int i = 0; i<current.size; ++i){
            delta[i] = get_delta(box, current.box[i]);
            if(delta[i] < delta[best]){
                best = i;
            }
        }
        return best;
    }

    void expand_box(Box & expand, Box const& include){
        for(int j = 0; j<DIM; ++j){
            if(include.mins[j] < expand.mins[j]) expand.mins[j] = include.mins[j];
            if(include.maxs[j] > expand.maxs[j]) expand.maxs[j] = include.maxs[j];
        }
    }

    Box split(Node & A, Box & box, Pointer & child){ // Tiene 4 outputs esta función
        for(int i = 0; i<ORDER; ++i){
            obox[i] = A.box[i];
            ochild[i] = A.child[i];
            oidx[i] = i;
        }
        obox[ORDER] = box;
        ochild[ORDER] = child;
        oidx[ORDER] = ORDER;

        child = create_node();
        Node & B = get_node(child); B.size = 0;

        int big_i, big_j;
        Position big_sz = INT32_MIN;

        for(int i = 0; i<ORDER; ++i){
            for(int j = i+1; j<=ORDER; ++j){
                Position size = 0;
                for(int k = 0; k<DIM; ++k){
                    size += MAX(obox[i].maxs[k], obox[j].maxs[k]) - MIN(obox[i].mins[k], obox[j].mins[k]);
                }
                size = get_delta(obox[i], obox[j]);
                if(big_sz < size){
                    big_i = i;
                    big_j = j;
                    big_sz = size;
                }
            }
        }

        A.box[0] = obox[big_i]; A.child[0] = ochild[big_i]; A.size = 1;
        B.box[0] = obox[big_j]; B.child[0] = ochild[big_j]; B.size = 1;

        Box box_A = A.box[0];
        box = B.box[0];
        Box & box_B = box;

        if(big_i < (ORDER-1)){
            if(big_j < (ORDER-1)){
                oidx[big_i] = oidx[ORDER];
                oidx[big_j] = oidx[ORDER - 1];
            }else{
                oidx[big_i] = (big_j != ORDER) ? oidx[ORDER] : oidx[ORDER - 1];
            }
        }

        for(int i = (ORDER - 1); i>0; --i){
            Position big_delta = INT32_MIN;
            int goto_A;
            for(int j = 0; j<i; ++j){
                Position const delta_A = get_delta(obox[oidx[j]], box_A);
                Position const delta_B = get_delta(obox[oidx[j]], box_B);
                if(delta_A <= delta_B){
                    Position const delta_diff = delta_B - delta_A;
                    if(delta_diff > big_delta){
                        big_delta = delta_diff;
                        big_j = j;
                        goto_A = 1;
                    }
                }else{
                    Position const delta_diff = delta_A - delta_B;
                    if(delta_diff > big_delta){
                        big_delta = delta_diff;
                        big_j = j;
                        goto_A = 0;
                    }
                }
            }
            if(goto_A){
                A.box[A.size] = obox[oidx[big_j]];
                A.child[A.size] = ochild[oidx[big_j]];
                for(int k = 0; k<DIM; ++k){
                    if(box_A.mins[k] > A.box[A.size].mins[k]) box_A.mins[k] = A.box[A.size].mins[k];
                    if(box_A.maxs[k] < A.box[A.size].maxs[k]) box_A.maxs[k] = A.box[A.size].maxs[k];
                }
                A.size += 1;
            }else{
                B.box[B.size] = obox[oidx[big_j]];
                B.child[B.size] = ochild[oidx[big_j]];
                for(int k = 0; k<DIM; ++k){
                    if(box_B.mins[k] > B.box[B.size].mins[k]) box_B.mins[k] = B.box[B.size].mins[k];
                    if(box_B.maxs[k] < B.box[B.size].maxs[k]) box_B.maxs[k] = B.box[B.size].maxs[k];
                }
                B.size += 1;
            }
            oidx[big_j] =  oidx[i-1];
        }

        return box_A;

    }


    vHBF get_neighbor_box(Point const& point, Pointer node_ptr, int i){
        Node const& node = get_node(node_ptr);
        Box const& box = node.box[i];
        Position distance = 0;
        for(int j = 0; j<DIM; ++j){
            Position const diff =   ( point[j] < box.mins[j] ) ? box.mins[j] - point[j] :
                                    ( point[j] > box.maxs[j] ) ? point[j] - box.maxs[j] : 0;
            distance += diff*diff;
        }
        return {distance, node.child[i]};
    }


    vKNN get_neighbor_poly(Point const& point, Pointer polygon_ptr, Pointer node_ptr, int i){
        // Temporal: distance to mmb
        Node const& node = get_node(node_ptr);
        Box const& box = node.box[i];
        Position distance = 0;
        Point polygon_point;
        for(int j = 0; j<DIM; ++j){
            if(point[j] < box.mins[j]){
                polygon_point[j] = box.mins[j];
                Position const diff =  box.mins[j] - point[j];
                distance += diff*diff;
            }else if(point[j] > box.maxs[j]){
                polygon_point[j] = box.maxs[j];
                Position const diff = point[j] - box.maxs[j];
                distance += diff*diff;
            }else{
                polygon_point[j] = point[j];
            }
        }
        return {distance, polygon_ptr, polygon_point};
    }


    bool is_not_polygon(Pointer ptr){
        return ptr < POLYGON_ZONE;
    }


    int get_first_inter(Node const& node, Box const& box){
        for(int i = 0; i<node.size; ++i){
            bool intersects = true;
            for(int j = 0; j<DIM; ++j){
                intersects = intersects && (node.box[i].mins[j] <= box.maxs[j] && node.box[i].maxs[j] >= box.mins[j]);
            }
            if(intersects) return i;
        }
        return -1;
    }


    void insert_helper(Box box, Pointer child){
        Pointer current = root;
        int parent = -1;

        while(is_not_leaf(current)){
            Node & node = get_node(current);
            int best = get_best_box(node, box);
            expand_box(node.box[best], box);
            parent += 1;
            parents[parent] = current;
            childs[parent] = best;
            current = node.child[best];
        }

        Box fix_box;
        while(true) {
            Node& current_node = get_node(current);
            if(current_node.size < ORDER){
                current_node.box[current_node.size] = box;
                current_node.child[current_node.size] = child;
                current_node.size += 1;
                return;
            }

            fix_box = split(current_node, box, child);

            if(parent == -1) break;

            Node & parent_node = get_node(parents[parent]);
            parent_node.box[childs[parent]] = fix_box;

            current = parents[parent--];

        }

        Pointer new_root = create_node();
        Node & root_node = get_node(new_root);

        root_node.box[0] = fix_box;
        root_node.box[1] = box;

        root_node.child[0] = root;
        root_node.child[1] = child;

        root_node.size = 2;

        root = new_root;
    }

    void update_parents_after_removal(int parent){
        for(int i = parent; i>0; --i){
            Node & current_node = get_node(parents[i]);
            Node & parent_node = get_node(parents[i-1]);

            Box & child_box = current_node.box[childs[i]];
            Box & current_box = parent_node.box[childs[i-1]];
            for(int k = 0; k<DIM; ++k){
                current_box.maxs[k] = INT32_MIN;
                current_box.mins[k] = INT32_MAX;
            }
            for(int j = 0; j<current_node.size; ++j){
                for(int k = 0; k<DIM; ++k){
                    current_box.maxs[k] = MAX(current_box.maxs[k], current_node.box[j].maxs[k]);
                    current_box.mins[k] = MIN(current_box.mins[k], current_node.box[j].mins[k]);
                }
            }
        }
    }



    void reinsert_except(int parent, Pointer except){
        size--;

        Node & node = get_node(parents[parent]);
        Pointer reinsert = node.child[childs[parent]];
        node.size -= 1;
        for(int i = childs[parent]; i<node.size; ++i){
            node.box[i] = node.box[i+1];
            node.child[i] = node.child[i+1];
        }

        update_parents_after_removal(parent);

        if(parent == 0 && node.size == 0) node.child[0] = POLYGON_ZONE;
        if(reinsert == except){
            destroy_polygon(except);
            return;
        }

        top_willbe_dirty = 0;
        bfs.clear();
        bfs.push({reinsert, 0});
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& top_node = get_node(top.node_ptr);
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i < top_node.size; ++i) bfs.push({top_node.child[i], 0});
                destroy_node(top.node_ptr);
            }else{
                willbe_dirty[top_willbe_dirty++] = top.node_ptr;
            }
            bfs.pop();
        }

        for(int i = 0; i<top_willbe_dirty; ++i){
            Pointer const& wptr = willbe_dirty[i];
            Node const& wnode = get_node(wptr);
            for(int j = 0; j < wnode.size; ++j){
                if(wnode.child[j] != except) insert_helper(wnode.box[j], wnode.child[j]);
            }
            destroy_node(wptr);
        }

        destroy_polygon(except);
    }



public:
    RTree(){
        top_dirty_node = 0;
        top_dirty_polygon = 0;

        for(Pointer i = 0; i < MAX_NODES; ++i) dirty_nodes[i] = i;
        for(Pointer i = 0; i < MAX_POLYGONS; ++i) dirty_polygons[i] = i;

        root = create_node();
        get_node(root).child[0] = POLYGON_ZONE;
    }

    void insert(Polygon const& polygon){
        size++;
        Box box = get_mbb(polygon);
        Pointer child = create_polygon(polygon);

        insert_helper(box, child);
    }

    void print(){
        bfs.clear();
        bfs.push({root, 0});
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& node = get_node(top.node_ptr);
            //std::cout << "{  ";
            for(int i = 0; i<node.size; ++i) {
                std::cout << " R" << i << "( ";
                for(int j = 0; j<DIM; ++j){
                    std::cout << 'D' <<  j << "[" << node.box[i].mins[j] << ',' << node.box[i].maxs[j] << "] ";
                }
                std::cout << ") ";
            }
            //std::cout << " }";
            std::cout << "\n\n";
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i<node.size; ++i) {
                    bfs.push( {node.child[i], 0} );
                }
            }
            bfs.pop();
        }
    }

    void for_each_polygon(std::function<void(Polygon const&)> const& f){
        for(int i = 0; polygons[i].size > 0; ++i) {
            if(polygons[i].size != DIRTY) f(polygons[i]);
        }
    }

    void for_each_box(std::function<void(Box const&, int, int)> const& f){
        int const height =  ceil(log2(size+1)/log2ORDER) + 1;
        bfs.clear();
        bfs.push({root, 0});
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& node = get_node(top.node_ptr);
            int const curr_lvl = top.lvl;
            for(int i = 0; i<node.size; ++i) {
                f(node.box[i], curr_lvl, height);
            }
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i<node.size; ++i) {
                    bfs.push( {node.child[i], curr_lvl + 1} );
                }
            }
            bfs.pop();
        }
    }

    void for_each_nearest(int k, Point const& point, std::function<void(Polygon const&, Point const&, Point const&, int distance)> const& f){
        knn.init(k, {INT32_MAX});
        hbf.clear();

        hbf.push({0, root});
        while(hbf.not_empty()){
            vHBF top = hbf.top();
            hbf.pop();
            auto const& dist2box = top.distance;
            auto const& worstbest =  knn.top().distance;
            if(dist2box < worstbest){
                Node const& node = get_node(top.node_ptr);
                if(is_not_leaf(top.node_ptr)){
                    for(int i = 0; i<node.size; ++i) {
                        auto const neighbor_box = get_neighbor_box(point, top.node_ptr, i);
                        if(neighbor_box.distance < worstbest){
                            hbf.push( neighbor_box );
                        }
                    }
                }else{
                    for(int i = 0; i<node.size; ++i) {
                        auto const& new_worstbest =  knn.top().distance;
                        auto const neighbor_box = get_neighbor_box(point, top.node_ptr, i);
                        if(neighbor_box.distance < new_worstbest){
                            auto const neighbor = get_neighbor_poly(point, node.child[i], top.node_ptr, i);
                            if(neighbor.distance < new_worstbest){
                                knn.pop();
                                knn.push( neighbor );
                            }
                        }
                    }
                }
            }

        }


        auto const* const neighbors = knn.data();

        for(int i = 0; i<k; ++i){
            auto const& neighbor = neighbors[i];
            if(neighbor.distance == INT32_MAX) continue;

            f(get_polygon(neighbor.polygon_ptr), point, neighbor.point, neighbor.distance);
        }


    }

    int get_size() {return size;}

    /*
        1. Bajar por primera interseccion
            1.1. Si ninguno intersecta, terminar
        2. Retroceder hasta el primer padre con tamaño mayor a 1
            2.1. Si ninguno cumple, recrear el arbol
            2.2. Si alguno cumple
                2.2.1. Eliminar todos los nodos internos
                2.2.1. Reinsertar todos los nodos hojas de ese ancestro
            (No reinsertar el que se debe de eliminar)
    */

    void erase(Point const& point){
        Box box;
        for(int i = 0; i<DIM; ++i){
            box.mins[i] = point[i] - 2;
            box.maxs[i] = point[i] + 2;
        }

        Pointer current = root;
        int parent = -1;

        while(is_not_polygon(current)){
            Node & node = get_node(current);
            int best = get_first_inter(node, box);
            if(best == -1) return;
            parent += 1;
            parents[parent] = current;
            childs[parent] = best;
            sizes[parent] = node.size;
            current = node.child[best];
        }



        while(parent > 0 && get_node(parents[parent]).size == 1) parent--;

        reinsert_except(parent, current);
    }

};

template<unsigned ORDER, unsigned DIM, unsigned MAX_POLYGONS, unsigned MAX_VERTEX, unsigned MAX_KNN>
class RTree<ORDER,DIM,MAX_POLYGONS,MAX_VERTEX,MAX_KNN,true>{
public:

    static constexpr unsigned MAX_NODES = MAX_POLYGONS * ORDER;

    static constexpr unsigned POLYGON_ZONE = MAX_NODES;

    static constexpr uint32_t DIRTY = UINT32_MAX;

    using Position = int32_t;
    using Pointer = uint32_t;
    using Size = uint32_t;
    using Identity = uint32_t;
    using Point = std::array<Position, DIM>;

    struct Polygon{
        Identity id;
        Size size = 0;
        Point vertex[MAX_VERTEX];
        Position z;
    };


    struct Box{
        Position mins[DIM];
        Position maxs[DIM];
        Position z;
    };

    struct Node{
        Size size = 0;
        Box box[ORDER];
        Pointer child[ORDER];
        Pointer parent = DIRTY, right = DIRTY, left = DIRTY;
    };

private:
    inline static const float log2ORDER = log2(ORDER);
    int size;
    Pointer root;


    Pointer delta[ORDER];
    Pointer parents[MAX_HEIGHT];
    int childs[MAX_HEIGHT];
    Size sizes[MAX_HEIGHT];


    Box obox[ORDER+1];
    Pointer ochild[ORDER+1];
    int oidx[ORDER+1];


    Node nodes[MAX_NODES];
    Polygon polygons[MAX_POLYGONS];

    Size top_dirty_node;
    Pointer dirty_nodes[MAX_NODES];

    Size top_dirty_polygon;
    Pointer dirty_polygons[MAX_POLYGONS];

    Size top_willbe_dirty;
    Pointer willbe_dirty[MAX_POLYGONS];

    struct vBFS{Pointer node_ptr; int lvl;};
    StaticQueue<vBFS, MAX_POLYGONS> bfs;

    struct vKNN{Position distance; Pointer polygon_ptr; Point point; const bool operator<(vKNN const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MAXHEAP, vKNN, MAX_KNN> knn;

    struct vHBF{Position distance; Pointer node_ptr; const bool operator<(vHBF const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MINHEAP, vHBF, MAX_HEIGHT*ORDER + 1> hbf; // H best first



    Node & get_node(Pointer node_ptr){
        return nodes[node_ptr];
    }
    Polygon & get_polygon(Pointer polygon_ptr){
        return polygons[polygon_ptr - POLYGON_ZONE];
    }
    Pointer create_node(){
        return dirty_nodes[top_dirty_node++];
    }
    void destroy_node(Pointer node_ptr){
        Node & node = get_node(node_ptr);
        node.left = DIRTY;
        node.right = DIRTY;
        node.parent = DIRTY;
        node.size = 0;
        dirty_nodes[--top_dirty_node] = node_ptr;
    }
    Pointer create_polygon(Polygon const& polygon){
        polygons[dirty_polygons[top_dirty_polygon]] = polygon;
        return dirty_polygons[top_dirty_polygon++] + POLYGON_ZONE;
    }
    void destroy_polygon(Pointer polygon_ptr){
        get_polygon(polygon_ptr).size = DIRTY;
        dirty_polygons[--top_dirty_polygon] = polygon_ptr - POLYGON_ZONE;
    }
    bool is_not_leaf(Pointer node_ptr){
        return nodes[node_ptr].child[0] < POLYGON_ZONE;
    }
    Box get_mbb(Polygon const& polygon){
        Box box;
        for(int j = 0; j<DIM; ++j) {
            box.mins[j] = polygon.vertex[0][j];
            box.maxs[j] = polygon.vertex[0][j];
        }
        for(int i = 1; i<polygon.size; ++i){
            for(int j = 0; j<DIM; ++j){
                if(box.mins[j] > polygon.vertex[i][j]) box.mins[j] = polygon.vertex[i][j];
                if(box.maxs[j] < polygon.vertex[i][j]) box.maxs[j] = polygon.vertex[i][j];
            }
        }
        box.z = polygon.z;
        return box;
    }

    Position get_delta(Box const& a, Box const& b){
        Position d = 0;
        for(int i = 0; i<DIM; ++i){
            if(a.mins[i] >= b.maxs[i]) d += (a.maxs[i] - b.maxs[i]);
            else if(a.maxs[i] <= b.mins[i]) d += (b.mins[i] - a.mins[i]);
            else{
                if(a.mins[i] < b.mins[i]) d += (b.mins[i] - a.mins[i]);
                if(a.maxs[i] > b.maxs[i]) d += (a.maxs[i] - b.maxs[i]);
            }
        }
        return d;
    }

    int get_best_box(Node& current, Box const& box){
        for(int i = 0; i<current.size; ++i){
            if(current.box[i].z > box.z) return i;
        }
        current.box[current.size-1].z = box.z;
        return current.size-1;
    }

    void expand_box(Box & expand, Box const& include){
        for(int j = 0; j<DIM; ++j){
            if(include.mins[j] < expand.mins[j]) expand.mins[j] = include.mins[j];
            if(include.maxs[j] > expand.maxs[j]) expand.maxs[j] = include.maxs[j];
        }
    }

    Box split(Pointer ptr_A, Box & box, Pointer & child){ // Tiene 4 outputs esta función
        Node & A = get_node(ptr_A);
        for(int i = 0; i<ORDER; ++i){
            obox[i] = A.box[i];
            ochild[i] = A.child[i];
        }
        {
            int i = ORDER - 1;
            for(;i>=0 && obox[i].z > box.z; --i){
                obox[i+1] = obox[i];
                ochild[i+1] = ochild[i];
            }
            obox[i+1] = box;
            ochild[i+1] = child;
        }


        child = create_node();
        Node & B = get_node(child); B.size = 0;
        {
            int i = 0;
            for(; i<=ORDER/2; ++i){
                A.box[i] = obox[i];
                A.child[i] = ochild[i];
            }
            A.size = i;
            for(; i<=ORDER; ++i){
                B.box[i-A.size] = obox[i];
                B.child[i-A.size] = ochild[i];
            }
            B.size = i - A.size;
        }

        Box box_A = A.box[0];
        box = B.box[0];
        Box & box_B = box;

        for(int i = 1; i<A.size; ++i) expand_box(box_A, A.box[i]);
        for(int i = 1; i<B.size; ++i) expand_box(box_B, B.box[i]);
        if (A.right == DIRTY) {
            B.parent = A.parent;
            B.right = A.right;
            B.left = ptr_A;
            A.right = child;
            return box_A;
        }
        Node & C = get_node(A.right);
        B.parent = A.parent;
        B.right = A.right;
        B.left = ptr_A;
        C.left = child;
        A.right = child;

        return box_A;
    }


    vHBF get_neighbor_box(Point const& point, Pointer node_ptr, int i){
        Node const& node = get_node(node_ptr);
        Box const& box = node.box[i];
        Position distance = 0;
        for(int j = 0; j<DIM; ++j){
            Position const diff =   ( point[j] < box.mins[j] ) ? box.mins[j] - point[j] :
                                    ( point[j] > box.maxs[j] ) ? point[j] - box.maxs[j] : 0;
            distance += diff*diff;
        }
        return {distance, node.child[i]};
    }


    vKNN get_neighbor_poly(Point const& point, Pointer polygon_ptr, Pointer node_ptr, int i){
        // Temporal: distance to mmb
        Node const& node = get_node(node_ptr);
        Box const& box = node.box[i];
        Position distance = 0;
        Point polygon_point;
        for(int j = 0; j<DIM; ++j){
            if(point[j] < box.mins[j]){
                polygon_point[j] = box.mins[j];
                Position const diff =  box.mins[j] - point[j];
                distance += diff*diff;
            }else if(point[j] > box.maxs[j]){
                polygon_point[j] = box.maxs[j];
                Position const diff = point[j] - box.maxs[j];
                distance += diff*diff;
            }else{
                polygon_point[j] = point[j];
            }
        }
        return {distance, polygon_ptr, polygon_point};
    }


    bool is_not_polygon(Pointer ptr){
        return ptr < POLYGON_ZONE;
    }


    int get_first_inter(Node const& node, Box const& box){
        for(int i = 0; i<node.size; ++i){
            bool intersects = true;
            for(int j = 0; j<DIM; ++j){
                intersects = intersects && (node.box[i].mins[j] <= box.maxs[j] && node.box[i].maxs[j] >= box.mins[j]);
            }
            if(intersects) return i;
        }
        return -1;
    }

    int find_child(Node const& node, Pointer child){
        for(int i = 0; i<node.size; ++i) if(node.child[i] == child) return i;
        return -1;
    }

    void update_parents_after_lend(Pointer current){
        for(Pointer parent = get_node(current).parent; parent != DIRTY;){
            Node & current_node = get_node(current);
            Node & parent_node = get_node(parent);

            Box & current_box = parent_node.box[find_child(parent_node, current)];
            for(int k = 0; k<DIM; ++k){
                current_box.maxs[k] = INT32_MIN;
                current_box.mins[k] = INT32_MAX;
            }
            for(int j = 0; j<current_node.size; ++j){
                for(int k = 0; k<DIM; ++k){
                    current_box.maxs[k] = MAX(current_box.maxs[k], current_node.box[j].maxs[k]);
                    current_box.mins[k] = MIN(current_box.mins[k], current_node.box[j].mins[k]);
                }
            }
            current = parent;
            parent = parent_node.parent;
        }
    }

    void lend_overflow_r(Pointer current, Box const& box, Pointer const& child){
        Node & node = get_node(current);
        if(node.size < ORDER){
            int i = node.size-1;
            for(; i>=0 && node.box[i].z > box.z; --i){
                node.box[i+1] = node.box[i];
                node.child[i+1] =node.child[i];
            }
            node.box[i+1] = box;
            node.child[i+1] = child;
            node.size += 1;
            update_parents_after_lend(current);
        }
        else if(node.box[0].z < box.z){
            Box ob = node.box[0];
            Pointer oc = node.child[0];
            int i = 1;
            for(; i<node.size && node.box[i].z < box.z; ++i){
                node.box[i-1] = node.box[i];
                node.child[i-1] = node.child[i];

            }
            node.box[i-1] = box;
            node.child[i-1] = child;
            //for(++i; i<node.size; ++i){
            //    node.box[i-1] = node.box[i];
            //    node.child[i-1] = node.child[i];
            //}
            update_parents_after_lend(current);
            lend_overflow_r(node.left, ob, oc);
        }
        else lend_overflow_r(node.left, box, child);
    }

    bool lend_overflow(Pointer current, Box const& box, Pointer const& child){
        Node & node = get_node(current);
        Pointer spointer = node.left;
        do {
            if(spointer == DIRTY) return false;
            Node & snode = get_node(spointer);
            if(snode.size < ORDER) break;
            spointer = snode.left;

        }while(true);

        lend_overflow_r(current, box, child);
        return true;
    }


    void insert_helper(Box box, Pointer child){
        Pointer current = root;
        int parent = -1;

        while(is_not_leaf(current)){
            Node & node = get_node(current);
            int best = get_best_box(node, box);
            expand_box(node.box[best], box);
            parent += 1;
            parents[parent] = current;
            childs[parent] = best;
            current = node.child[best];
        }

        Box fix_box;
        while(true) {
            Node& current_node = get_node(current);
            if(current_node.size < ORDER){
                int i = current_node.size-1;
                for(; i>=0 && current_node.box[i].z > box.z; --i){
                    current_node.box[i+1] = current_node.box[i];
                    current_node.child[i+1] =current_node.child[i];
                }
                current_node.box[i+1] = box;
                current_node.child[i+1] = child;
                current_node.size += 1;
                return;
            }

            if(lend_overflow(current, box, child)) return;

            fix_box = split(current, box, child);

            if(parent == -1) break;

            Node & parent_node = get_node(parents[parent]);
            parent_node.box[childs[parent]] = fix_box;

            current = parents[parent--];

        }

        Pointer new_root = create_node();
        Node & root_node = get_node(new_root);

        root_node.box[0] = fix_box;
        root_node.box[1] = box;

        root_node.child[0] = root;
        root_node.child[1] = child;

        root_node.size = 2;

        root_node.parent = DIRTY;
        get_node(root).parent = new_root;
        get_node(child).parent = new_root;

        root = new_root;
    }

    void update_parents_after_removal(int parent){
        for(int i = parent; i>0; --i){
            Node & current_node = get_node(parents[i]);
            Node & parent_node = get_node(parents[i-1]);

            Box & child_box = current_node.box[childs[i]];
            Box & current_box = parent_node.box[childs[i-1]];
            for(int k = 0; k<DIM; ++k){
                current_box.maxs[k] = INT32_MIN;
                current_box.mins[k] = INT32_MAX;
            }
            for(int j = 0; j<current_node.size; ++j){
                for(int k = 0; k<DIM; ++k){
                    current_box.maxs[k] = MAX(current_box.maxs[k], current_node.box[j].maxs[k]);
                    current_box.mins[k] = MIN(current_box.mins[k], current_node.box[j].mins[k]);
                }
            }
        }
    }



    void reinsert_except(int parent, Pointer except){
        size--;

        Node & node = get_node(parents[parent]);
        Pointer reinsert = node.child[childs[parent]];
        node.size -= 1;
        for(int i = childs[parent]; i<node.size; ++i){
            node.box[i] = node.box[i+1];
            node.child[i] = node.child[i+1];
        }

        update_parents_after_removal(parent);

        if(parent == 0 && node.size == 0) node.child[0] = POLYGON_ZONE;
        if(reinsert == except){
            destroy_polygon(except);
            return;
        }

        top_willbe_dirty = 0;
        bfs.clear();
        bfs.push({reinsert, 0});
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& top_node = get_node(top.node_ptr);
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i < top_node.size; ++i) bfs.push({top_node.child[i], 0});
                destroy_node(top.node_ptr);
            }else{
                willbe_dirty[top_willbe_dirty++] = top.node_ptr;
            }
            bfs.pop();
        }

        for(int i = 0; i<top_willbe_dirty; ++i){
            Pointer const& wptr = willbe_dirty[i];
            Node const& wnode = get_node(wptr);
            for(int j = 0; j < wnode.size; ++j){
                if(wnode.child[j] != except) insert_helper(wnode.box[j], wnode.child[j]);
            }
            destroy_node(wptr);
        }

        destroy_polygon(except);
    }



public:
    RTree(){
        top_dirty_node = 0;
        top_dirty_polygon = 0;

        for(Pointer i = 0; i < MAX_NODES; ++i) dirty_nodes[i] = i;
        for(Pointer i = 0; i < MAX_POLYGONS; ++i) dirty_polygons[i] = i;

        root = create_node();
        get_node(root).child[0] = POLYGON_ZONE;
    }

    void insert(Polygon const& polygon){
        size++;
        Box box = get_mbb(polygon);
        Pointer child = create_polygon(polygon);

        insert_helper(box, child);
    }

    void print(){
        bfs.clear();
        bfs.push({root, 0});
        std::cout << "HILBERT R TREE:\n";
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& node = get_node(top.node_ptr);
            //std::cout << "{  ";
            for(int i = 0; i<node.size; ++i) {
                std::cout << " R" << i << "( ";
                for(int j = 0; j<DIM; ++j){
                    std::cout << 'D' <<  j << "[" << node.box[i].mins[j] << "] ";

                }
                std::cout << "Z[" << node.box[i].z << "] ";
                std::cout << ") ";
            }
            //std::cout << " }";
            std::cout << "\n\n";
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i<node.size; ++i) {
                    bfs.push( {node.child[i], 0} );
                }
            }
            bfs.pop();
        }
    }

    void for_each_polygon(std::function<void(Polygon const&)> const& f){
        for(int i = 0; polygons[i].size > 0; ++i) {
            if(polygons[i].size != DIRTY) f(polygons[i]);
        }
    }

    void for_each_box(std::function<void(Box const&, int, int)> const& f){
        int const height =  ceil(log2(size+1)/log2ORDER) + 1;
        bfs.clear();
        bfs.push({root, 0});
        while(bfs.not_empty()) {
            auto const& top = bfs.top();
            Node const& node = get_node(top.node_ptr);
            int const curr_lvl = top.lvl;
            for(int i = 0; i<node.size; ++i) {
                f(node.box[i], curr_lvl, height);
            }
            if(is_not_leaf(top.node_ptr)){
                for(int i = 0; i<node.size; ++i) {
                    bfs.push( {node.child[i], curr_lvl + 1} );
                }
            }
            bfs.pop();
        }
    }

    void for_each_nearest(int k, Point const& point, std::function<void(Polygon const&, Point const&, Point const&, int distance)> const& f){
        knn.init(k, {INT32_MAX});
        hbf.clear();

        hbf.push({0, root});
        while(hbf.not_empty()){
            vHBF top = hbf.top();
            hbf.pop();
            auto const& dist2box = top.distance;
            auto const& worstbest =  knn.top().distance;
            if(dist2box < worstbest){
                Node const& node = get_node(top.node_ptr);
                if(is_not_leaf(top.node_ptr)){
                    for(int i = 0; i<node.size; ++i) {
                        auto const neighbor_box = get_neighbor_box(point, top.node_ptr, i);
                        if(neighbor_box.distance < worstbest){
                            hbf.push( neighbor_box );
                        }
                    }
                }else{
                    for(int i = 0; i<node.size; ++i) {
                        auto const& new_worstbest =  knn.top().distance;
                        auto const neighbor_box = get_neighbor_box(point, top.node_ptr, i);
                        if(neighbor_box.distance < new_worstbest){
                            auto const neighbor = get_neighbor_poly(point, node.child[i], top.node_ptr, i);
                            if(neighbor.distance < new_worstbest){
                                knn.pop();
                                knn.push( neighbor );
                            }
                        }
                    }
                }
            }

        }


        auto const* const neighbors = knn.data();

        for(int i = 0; i<k; ++i){
            auto const& neighbor = neighbors[i];
            if(neighbor.distance == INT32_MAX) continue;

            f(get_polygon(neighbor.polygon_ptr), point, neighbor.point, neighbor.distance);
        }


    }

    int get_size() {return size;}

    /*
        1. Bajar por primera interseccion
            1.1. Si ninguno intersecta, terminar
        2. Retroceder hasta el primer padre con tamaño mayor a 1
            2.1. Si ninguno cumple, recrear el arbol
            2.2. Si alguno cumple
                2.2.1. Eliminar todos los nodos internos
                2.2.1. Reinsertar todos los nodos hojas de ese ancestro
            (No reinsertar el que se debe de eliminar)
    */

    void erase(Point const& point){
        Box box;
        for(int i = 0; i<DIM; ++i){
            box.mins[i] = point[i] - 2;
            box.maxs[i] = point[i] + 2;
        }

        Pointer current = root;
        int parent = -1;

        while(is_not_polygon(current)){
            Node & node = get_node(current);
            int best = get_first_inter(node, box);
            if(best == -1) return;
            parent += 1;
            parents[parent] = current;
            childs[parent] = best;
            sizes[parent] = node.size;
            current = node.child[best];
        }



        while(parent > 0 && get_node(parents[parent]).size == 1) parent--;

        reinsert_except(parent, current);
    }

};

