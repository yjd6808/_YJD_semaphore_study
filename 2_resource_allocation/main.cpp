#include <thread>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>

using namespace std;

template <int _t>
class pool {

    using t_pool = pool<_t>;
public:
    void initialize() {
        v.emplace_back(this);
        v.emplace_back(this);
        v.emplace_back(this);
        v.emplace_back(this);
        v.emplace_back(this);
        v.emplace_back(this);

        for (int i = 0; i < v.size(); i++)
            v[i].th.join();
    }
private:
    struct _th {
        thread th;
        t_pool* parent;

        _th(t_pool* pool): th(routine, ref(*this)), parent(pool) {

        }
    
        private:
        static void routine(_th& this_) {
            auto& z = this_.parent->v[0];
            if (this_.parent->v.size() > 0)
                cout <<"zz\n";
            cout << typeid(z).name() <<"\n";
            cout << "hello world\n";
        }
    };
  
    vector<_th> v; 
};


int main() {
    pool<5> p;
    p.initialize();
    return 0;
}
