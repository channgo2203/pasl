#include "rake-compress-primitives.hpp"
#include <unordered_map>

#ifdef SPECIAL
loop_controller_type update_loop1("update_loop1");
loop_controller_type update_loop2("update_loop2");
loop_controller_type update_loop3("update_loop3");
loop_controller_type update_loop4("update_loop4");
loop_controller_type update_loop5("update_loop5");
loop_controller_type update_loop6("update_loop6");
loop_controller_type update_loop7("update_loop7");
#endif

int* ids;
std::unordered_set<Node*>* old_live_affected_sets;
std::unordered_set<Node*>* old_deleted_affected_sets;

void initialization_update_seq(int n, int add_no, int* add_p, int* add_v, int delete_no, int* delete_p, int* delete_v) {
  set_number = 1;
  vertex_thread = new int[n];
  for (int i = 0; i < n; i++) {
    vertex_thread[i] = -1;
  }

  ids = new int[set_number];
  ids[0] = 0;

  live_affected_sets = new std::unordered_set<Node*>[set_number];
  deleted_affected_sets = new std::unordered_set<Node*>[set_number];
  live_affected_sets[0] = std::unordered_set<Node*>();
  deleted_affected_sets[0] = std::unordered_set<Node*>();

  old_live_affected_sets = new std::unordered_set<Node*>[set_number];
  old_deleted_affected_sets = new std::unordered_set<Node*>[set_number];
  old_live_affected_sets[0] = std::unordered_set<Node*>();
  old_deleted_affected_sets[0] = std::unordered_set<Node*>();

  for (int i = 0; i < delete_no; i++) {
    Node* p = lists[delete_p[i]]->head;
    Node* v = lists[delete_v[i]]->head;

    make_affected(p, 0, false);
    make_affected(v, 0, false);

    v->set_parent(v);
    p->remove_child(v);
  }

  for (int i = 0; i < add_no; i++) {
    Node* p = lists[add_p[i]]->head;
    Node* v = lists[add_v[i]]->head;

    make_affected(p, 0, false);
    make_affected(v, 0, false);

    v->set_parent(p);
    p->add_child(v);
  }
}


// TODO: change to more parallelizable structure, as list of adjacency
void initialization_update(int n, std::unordered_map<int, std::vector<std::pair<int, bool>>> add,
                           std::unordered_map<int, std::vector<std::pair<int, bool>>> del) {
  set_number = add.size() + del.size();

  ids = new int[set_number];
  for (int i = 0; i < set_number; i++) {
    ids[i] = i;
  }

  vertex_thread = new int[n];
  for (int i = 0; i < n; i++) {
    vertex_thread[i] = -1;
  }

  live_affected_sets = new std::unordered_set<Node*>[set_number];
  deleted_affected_sets = new std::unordered_set<Node*>[set_number];
  old_live_affected_sets = new std::unordered_set<Node*>[set_number];
  old_deleted_affected_sets = new std::unordered_set<Node*>[set_number];
  for (int i = 0; i < set_number; i++) {
    live_affected_sets[i] = std::unordered_set<Node*>();
    deleted_affected_sets[i] = std::unordered_set<Node*>();
    old_live_affected_sets[i] = std::unordered_set<Node*>();
    old_deleted_affected_sets[i] = std::unordered_set<Node*>();
  }

  for (auto p : del) {
    int v = p.first;
    for (auto u : p.second) {
      if (u.second) {
        lists[v]->head->set_parent(lists[v]->head);
      } else {
        lists[v]->head->remove_child(lists[u.first]->head);
      }
    }
  }

  for (auto p : add) {
    int v = p.first;
    for (auto u : p.second) {
      if (u.second) {
        lists[v]->head->set_parent(lists[u.first]->head);
      } else {
        lists[v]->head->add_child(lists[u.first]->head);
      }
    }
  }

  int id = 0;
  for (auto p : del) {
    make_affected(lists[p.first]->head, id++, false);
    if (lists[p.first]->head->degree() == 0 ^ lists[p.first]->head->is_singleton()) {
      make_affected(lists[p.first]->head->get_parent(), id - 1, false);
    }
  }

  for (auto p : add) {
    make_affected(lists[p.first]->head, id++, false);
    if (lists[p.first]->head->degree() == 0 ^ lists[p.first]->head->is_singleton()) {
//      std::cerr << " " << p.first << " " << lists[p.first]->head->get_parent() << " " << lists[0]->head << std::endl;
      make_affected(lists[p.first]->head->get_parent(), id - 1, false);
    }
  }

  for (int i = 0; i < id; i++) {
    for (Node* v : live_affected_sets[i]) {
      v->state.singleton = v->degree() == 0;
    }
  }

  live[0] = new int[id];
  for (int i = 0; i < id; i++) {
    live[0][i] = i;
  }
  len[0] = id;
  live[1] = new int[id];
}

void initialization_update(int n, int add_no, int* add_p, int* add_v, int delete_no, int* delete_p, int* delete_v) {
  std::unordered_map<int, std::vector<std::pair<int, bool>>> add;
  std::unordered_map<int, std::vector<std::pair<int, bool>>> del;

  for (int i = 0; i < add_no; i++) {
    if (add.count(add_p[i]) == 0) {
      add.insert({add_p[i], std::vector<std::pair<int, bool>>()});
    }
    add[add_p[i]].push_back({add_v[i], false});
    if (add.count(add_v[i]) == 0) {
      add.insert({add_v[i], std::vector<std::pair<int, bool>>()});
    }
    add[add_v[i]].push_back({add_p[i], true});
  }

  for (int i = 0; i < delete_no; i++) {
    if (del.count(delete_p[i]) == 0) {
      del.insert({delete_p[i], std::vector<std::pair<int, bool>>()});
    }
    del[delete_p[i]].push_back({delete_v[i], false});
    if (del.count(delete_v[i]) == 0) {
      del.insert({delete_v[i], std::vector<std::pair<int, bool>>()});
    }
    del[delete_v[i]].push_back({delete_p[i], true});
  }
  initialization_update(n, add, del);
}

void free_vertex(Node* v, int thread_id) {
#ifdef STANDART
  if (v->next != NULL) {
    deleted_affected_sets[thread_id].insert(v->next);
  }
#endif
  v->next = NULL;
  lists[v->get_vertex()] = v;
  vertex_thread[v->get_vertex()] = -1;
  v->prepare();
}

void update_round_seq(int round) {
  std::unordered_set<Node*> old_live_affected_set;
  std::unordered_set<Node*> old_deleted_affected_set;
  old_live_affected_set.swap(live_affected_sets[0]);
  old_deleted_affected_set.swap(deleted_affected_sets[0]);

  for (Node* v : old_live_affected_set) {
    ;
    v->state.frontier = on_frontier(v);
  }

  for (Node* v : old_live_affected_set) {
    v->state.contracted = is_contracted(v, round);
    if (v->state.frontier) {
      Node* p = v->get_parent();
      if (v->is_contracted() || p->is_affected()) {
        if (p->is_contracted() && v->is_contracted()) {
          make_affected(p->get_parent(), 0, true);
        }

        if (p->state.contracted = is_contracted(p, round)) {
          make_affected(p->get_parent(), 0, true);
          free_vertex(p, 0);
        } else {
          make_affected(p, 0, true);
        }
      }
#ifdef STANDART
      for (Node* child : v->get_children()) {
#elif SPECIAL
      Node** children = v->get_children();
      for (int i = 0; i < MAX_DEGREE; i++) {
        if (children[i] == NULL)
          continue;
        Node* child = children[i];
#endif
        if (v->is_contracted() || child->is_affected()) {
          if (child->state.contracted = is_contracted(child, round)) {
            free_vertex(child, 0);
          } else {
            make_affected(child, 0, true);
          }
        }
      }
    }
    if (!v->is_contracted() && !v->is_root()) {
      copy_node(v);
      live_affected_sets[0].insert(v->next);
    } else {
/*
      if (v->next != NULL) {
        deleted_affected_sets[0].insert(v->next);
      }
      lists[v->get_vertex()] = v;
      v->next = NULL;
      v->prepare();
      vertex_thread[v->get_vertex()] = -1;*/
      free_vertex(v, 0);
    }
  }

  for (Node* v : live_affected_sets[0]) {
    Node* p = v->get_parent();
    
    if (p->is_contracted()) {
      delete_node_for(p, v);
    }
#ifdef STANDART
    std::set<Node*>& copy_children = v->prev->get_children();
    for (Node* u : copy_children) {
#elif SPECIAL
    Node** children = v->prev->get_children();
    for (int i = 0; i < MAX_DEGREE; i++) {
      if (children[i] == NULL)
        continue;
      Node* u = children[i];
#endif
      if (u->is_contracted()) {
        delete_node_for(u, v);
      }
    }
  }

  for (Node* v : live_affected_sets[0]) {
    v->advance();
    v->prepare();
  }

  for (Node* v : old_deleted_affected_set) {
    if (v->next != NULL)
      deleted_affected_sets[0].insert(v->next);
    delete v;              
  }
}

void update_round(int round) {
/*  std::cerr << "affected: ";
  for (int i = 0; i < len[round % 2]; i++) {
    std::cerr << live[round % 2][i] << ": ";
    for (Node* v : live_affected_sets[live[round % 2][i]]) {
      std::cerr << v->get_vertex() << " ";
    }
    std::cerr << std::endl;
  }*/

//  for (int i = 0; i < set_number; i++) {
#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop1, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    old_live_affected_sets[i].clear();
    live_affected_sets[i].swap(old_live_affected_sets[i]);
    old_deleted_affected_sets[i].clear();
    deleted_affected_sets[i].swap(old_deleted_affected_sets[i]);
  });

//  std::cerr << "\nRound: " << round << "\n";
#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
//  pasl::sched::granularity::parallel_for(update_loop2, 0, len[round % 2], [&] (int j) {
  for (int j = 0; j < len[round % 2]; j++) {
#endif
    int i = live[round % 2][j];
    for (Node* v : old_live_affected_sets[i]) {
      bool contracted = v->state.contracted;
      v->state.contracted = is_contracted(v, round);
//      std::cerr << v->get_vertex() << std::endl;
      if (v->state.contracted || contracted) {
/*        if (v->state.contracted) {
          std::cerr << "Contract: " << v->get_vertex() << std::endl;
        }*/
        Node* p = v->get_parent();
        if (vertex_thread[p->get_vertex()] == -1) {
//          std::cerr << "Affect " << p->get_vertex() << std::endl;
          p->set_proposal(v, i);
        }
#ifdef STANDART
        for (Node* c : v->get_children()) {
#elif SPECIAL
        Node** children = v->get_children();
        for (int k = 0; k < MAX_DEGREE; k++) {
          if (children[k] == NULL)
            continue;
          Node* c = children[k];
#endif
          if (vertex_thread[c->get_vertex()] == -1) {
            c->set_proposal(v, i);
//            std::cerr << "Affect " << c->get_vertex() << std::endl;
          }
        }
      }

      if (!v->is_contracted() && !v->is_root()) {
        copy_node(v);
        live_affected_sets[i].insert(v->next);
      }
    }
  }//);

//  for (int i = 0; i < set_number; i++) {
#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop3, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    for (Node* v : old_live_affected_sets[i]) {
      Node* p = v->get_parent();
      if (get_thread_id(p) == i) {
        p->state.contracted = is_contracted(p, round);
        if (p->is_contracted()) {
          free_vertex(p, i);
        } else {
          make_affected(p, i, true);
        }
      }
#ifdef STANDART
      for (Node* u : v->get_children()) {
#elif SPECIAL
      Node** children = v->get_children();
      for (int k = 0; k < MAX_DEGREE; k++) {
        if (children[k] == NULL)
          continue;
        Node* u = children[k];
#endif
        if (get_thread_id(u) == i) {
          u->state.contracted = is_contracted(u, round);
          if (u->is_contracted()) {
            free_vertex(u, i);
          } else {
            make_affected(u, i, true);
          }
        }
      }
      if (vertex_thread[v->get_vertex()] == i && (v->is_contracted() || v->is_root())) {
        free_vertex(v, i);
      }
    }
  });

#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop4, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    for (Node* v : live_affected_sets[i]) {
//      std::cerr << v->get_vertex() << std::endl;
      if (v->get_parent()->is_contracted()) {
        delete_node_for(v->get_parent(), v);
      }
#ifdef STANDART
      std::set<Node*>& copy_children = v->prev->get_children();
      for (Node* c : copy_children) {
#elif SPECIAL
      Node** children = v->prev->get_children();
      for (int k = 0; k < MAX_DEGREE; k++) {
        if (children[k] == NULL)
          continue;
        Node* c = children[k];
#endif
        if (c->is_contracted()) {
          delete_node_for(c, v);
        }
       }
    }
  });

#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop5, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    for (Node* v : live_affected_sets[i]) {
      if (v->degree() == 0 ^ v->is_singleton()) {
        Node* p = v->get_parent();
        if (vertex_thread[p->get_vertex()] == -1) {
          p->next->set_proposal(v, i);
        }
      }
    }
  });

#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop6, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    old_live_affected_sets[i].clear();
    for (Node* v : live_affected_sets[i]) {
      if (v->degree() == 0 ^ v->is_singleton()) {
        Node* p = v->get_parent();
        if (get_thread_id(p->next) == i) {
          vertex_thread[p->get_vertex()] = i;
          old_live_affected_sets[i].insert(p->next);
        }
      }
      v->advance();
    }
    live_affected_sets[i].insert(old_live_affected_sets[i].begin(), old_live_affected_sets[i].end());
  });


#ifdef STANDART
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
#elif SPECIAL
  pasl::sched::granularity::parallel_for(update_loop7, 0, len[round % 2], [&] (int j) {
#endif
    int i = live[round % 2][j];
    for (Node* v : live_affected_sets[i]) {
      v->prepare();
    }
  });

#ifdef STANDART
//  for (int i = 0; i < set_number; i++) {
  pasl::sched::native::parallel_for(0, len[round % 2], [&] (int j) {
    int i = live[round % 2][j];
    for (Node* v : old_deleted_affected_sets[i]) {
      if (v->next != NULL)
        deleted_affected_sets[i].insert(v->next);
      delete v;
    }
  });
#endif

  len[1 - round % 2] = pbbs::sequence::filter(live[round % 2], live[1 - round % 2], len[round % 2], [&] (int i) {
    return live_affected_sets[i].size() + deleted_affected_sets[i].size() != 0;
  });
}

bool end_condition_seq(int round_no) {
  int cnt = 0;
  for (int i = 0; i < set_number; i++) {
    cnt += live_affected_sets[i].size() + deleted_affected_sets[i].size();
  }
  return cnt;
}

bool end_condition(int round_no) {
 return len[round_no % 2];//pbbs::sequence::plusReduce(ids, set_number, [&] (int i) { return live_affected_sets[i].size() + deleted_affected_sets[i].size();});
}

template <typename Round, typename Condition>
void update(int n, Round round_function, Condition condition_function) {
  int round_no = 0;
  while (condition_function(round_no) > 0) {
    round_function(round_no);
    round_no++;
  }

  std::cerr << "Number of rounds: " << round_no << std::endl;
}