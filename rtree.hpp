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

template<unsigned ORDER, unsigned DIM, unsigned MAX_POLYGONS, unsigned MAX_VERTEX, unsigned MAX_KNN>
class RTree{
    public:

    static constexpr unsigned MAX_NODES = MAX_POLYGONS * ORDER;

    static constexpr unsigned POLYGON_ZONE = MAX_NODES;

    static constexpr uint32_t DIRTY = UINT32_MAX;

    using Position = int32_t;
    using Pointer = uint32_t;
    using Size = uint32_t;
    using Identity = uint32_t;

    struct Polygon{
        Identity id;
        Size size = 0;
        Position vertex[MAX_VERTEX][DIM];
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
    Pointer root;
    Pointer delta[ORDER];
    Pointer parents[MAX_HEIGHT];
    int childs[MAX_HEIGHT];
    Box obox[ORDER+1];
    Pointer ochild[ORDER+1];
    int oidx[ORDER+1];
    int size;
    inline static const float log2ORDER = log2(ORDER);

    Node nodes[MAX_NODES];
    Polygon polygons[MAX_POLYGONS];

    Size top_dirty_node;
    Pointer dirty_nodes[MAX_NODES];

    Size top_dirty_polygon;
    Pointer dirty_polygons[MAX_POLYGONS];

    struct vBFS{Pointer node; int lvl;};
    StaticQueue<vBFS, MAX_POLYGONS> bfs;
    
    struct vKNN{Position distance; Pointer polygon; Position point[DIM]; const bool operator<(vKNN const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MAXHEAP, vKNN, MAX_KNN> knn;

    struct vHBF{Position distance; Pointer node; const bool operator<(vHBF const& other) const { return distance < other.distance; } };
    StaticPriorityQueue<MINHEAP, vHBF, MAX_HEIGHT> hbf; // H best first


    Pointer create_node(){
        return dirty_nodes[top_dirty_node++];
    }
    void destroy_node(Pointer node){
        dirty_nodes[--top_dirty_node] = node;
    }
    Pointer create_polygon(Polygon const& polygon){
        polygons[dirty_nodes[top_dirty_polygon]] = polygon;
        return dirty_nodes[top_dirty_polygon++] + POLYGON_ZONE;
    }
    void destroy_polygon(Pointer polygon){
        dirty_nodes[--top_dirty_polygon] = polygon;
    }
    bool is_not_leaf(Pointer node){
        return nodes[node].child[0] < POLYGON_ZONE;
    }
    Node & get_node(Pointer node){
        return nodes[node]; 
    }
    Polygon & get_polygon(Pointer polygon){
        return polygons[POLYGON_ZONE - polygon]; 
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


    vHBF get_dist2box(Box const& box){
        vHBF result;

        return result;
    }

    vKNN get_dist2poly(Pointer polygon){
        vKNN result;

        return result;
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
            Node & current_node = get_node(current);
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

    void print(){
        bfs.clear();
        bfs.push({root, 0});
        while(bfs.not_empty()) {
            Node const& node = get_node(bfs.top().node);
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
            if(is_not_leaf(bfs.top().node)){
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
            auto const& element = bfs.top();
            Node const& node = get_node(element.node);
            int const curr_lvl = element.lvl;
            for(int i = 0; i<node.size; ++i) {
                f(node.box[i], curr_lvl, height);
            }
            if(is_not_leaf(element.node)){
                for(int i = 0; i<node.size; ++i) {
                    bfs.push( {node.child[i], curr_lvl + 1} );
                }
            }
            bfs.pop();
        }
    }

    void for_each_nearest(int k, std::array<Position, DIM> const& pos, std::function<void(Polygon const&)> const& f){
        knn.init(k, {INT32_MAX});
        hbf.clear();

        hbf.push({0, root});
        while(hbf.not_empty()){
            auto const& element = hbf.top();
            auto const& dist2box = element.distance;
            auto const& worstbest =  knn.top().distance;
            if(dist2box < worstbest){
                Node const& node = get_node(element.node);
                if(is_not_leaf(element.node)){
                    for(int i = 0; i<node.size; ++i) {
                        auto const& candidate = get_dist2box(node.box[i]);
                        if(candidate.distance < worstbest){
                            hfs.push( {candidate, node.box[i]} );
                        }
                    }
                }else{
                    for(int i = 0; i<node.size; ++i) {
                        auto const& candidate = get_dist2poly(node.child[i]);
                        if(candidate.distance < knn.top().distance){
                            knn.push( candidate );
                        }
                    }
                }
            }
            hbf.pop();
        }


    }

};

