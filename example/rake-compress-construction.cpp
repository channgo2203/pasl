#include "cmdline.hpp"
#include "benchmark.hpp"
#include "rake-compress-construction-functions.hpp"

int main(int argc, char** argv) {
   bool seq;
   int n;
   auto init = [&] {
     n = (long)pasl::util::cmdline::parse_or_default_int("n", 24);
     std::string graph = pasl::util::cmdline::parse_or_default_string("graph", std::string("bamboo"));
     seq = pasl::util::cmdline::parse_or_default_int("seq", 1) == 1;

     std::vector<int>* children = new std::vector<int>[n];
     int* parent = new int[n];
     if (graph.compare("binary_tree") == 0) {
       // let us firstly think about binary tree
       for (int i = 0; i < n; i++) {
         parent[i] = i == 0 ? 0 : (i - 1) / 2;
         children[i] = std::vector<int>();
         if (2 * i + 1 < n) {
           children[i].push_back(2 * i + 1);
         }
         if (2 * i + 2 < n) {
           children[i].push_back(2 * i + 2);
         }
       }
     } else {
       // bambooooooo
       for (int i = 0; i < n; i++) {
         parent[i] = i == 0 ? 0 : i - 1;
         children[i] = std::vector<int>();
         if (i + 1 < n)
           children[i].push_back(i + 1);
       }
     }
  
     initialization_construction(n, children, parent);
     delete [] children;
     delete [] parent;
   };

   auto run = [&] (bool sequential) {
     if (seq) {
       std::cerr << "Sequential run" << std::endl;
       construction(n, [&] (int round_no) {construction_round_seq(round_no);});
     } else {
       std::cerr << "Parallel run" << std::endl;
       construction(n, [&] (int round_no) {construction_round(round_no);});
     }
   };

   auto output = [&] {
     std::cout << "the construction has finished." << std::endl;
   };

   auto destroy = [&] {
     delete [] live[0];
     delete [] live[1];

     for (int i = 0; i < n; i++) {
       Node* start = lists[i]->head;
       while (start != NULL) {
         Node* next = start->next;
         delete start;
         start = next;
       }
     }

     delete [] lists;
   };

   pasl::sched::launch(argc, argv, init, run, output, destroy);

   return 0;
}                