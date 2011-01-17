#include <scope/test.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include "compiler.h"
#include "utility_impl.h"
#include "states.h"
#include "MockCallback.h"

#include "test_helper.h"

/*
std::ostream& operator<<(std::ostream& os, const boost::shared_ptr<TransitionPtr>& p) {
  return os;
}
*/

/*
bool buildNFA(Graph& fsm, const std::string& dot) {
  std::istringstream is(dot);

  // Vertex properties
  typedef boost::property<boost::vertex_name_t, std::string> vertex_p;
  // Edge properties
  typedef boost::property<boost::edge_name_t, std::string> edge_p;
  // adjacency_list-based type
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                vertex_p, edge_p> graph_t;

  // Construct an empty graph and prepare the dynamic_property_maps
  graph_t src(0);
  boost::dynamic_properties dp;

  boost::property_map<graph_t, boost::vertex_name_t>::type node_label =
    get(boost::vertex_name, src);
  dp.property("node_id", node_label);

  boost::property_map<graph_t, boost::edge_name_t>::type edge_label =
    get(boost::edge_name, src);
  dp.property("label", edge_label);
  
  if (!boost::read_graphviz(is, src, dp, "node_id")) return false;

  // Convert this graph to a Graph (annoying!)
  
  typedef boost::graph_traits<graph_t>::vertex vertex_t;
  typedef boost::graph_traits<graph_t>::edge_iterator edge_iterator;

  edge_iterator e, e_end;
  for (boost::tie(e, e_end) = boost::edges(src); e != e_end; ++e) {
    const unsigned int u = boost::lexical_cast<unsigned int>(
      boost::get(node_label, boost::source(*e, src)));

    const unsigned int v = boost::lexical_cast<unsigned int>(
      boost::get(node_label, boost::target(*e, src)));

    const char lit = boost::lexical_cast<char>(boost::get(edge_label, *e));

    edge(u, v, fsm, new LitState(lit));
  }

  return true;
}
*/

std::ostream& operator<<(std::ostream& out, const StateLayoutInfo& state) {
  out << "(" << state.Start << ", " << state.NumEval << ", " << state.NumOther << ", " << state.CheckIndex << ")";
  return out;
}

SCOPE_TEST(acOrbcProgram) {
  Graph fsm(4);

  edge(0, 1, fsm, new LitState('a')); // ac|bc
  edge(0, 2, fsm, new LitState('b'));
  edge(1, 3, fsm, new LitState('c'));
  edge(2, 3, fsm, new LitState('c'));
  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  
  SCOPE_ASSERT_EQUAL(9u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLongFork(&prog[0], 5), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[2]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('c'), prog[3]);
  SCOPE_ASSERT_EQUAL(Instruction::makeCheckHalt(1), prog[4]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[5]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[6]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[7], 2), prog[7]);
}

SCOPE_TEST(keywordLabels) {
  Graph fsm(4);
  edge(0, 1, fsm, new LitState('a', 0));
  edge(0, 2, fsm, new LitState('b'));
  edge(2, 3, fsm, new LitState('c', 1));
  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);

  SCOPE_ASSERT_EQUAL(11u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLongFork(&prog[0], 6), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[2]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[3]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[4]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[5]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[6]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('c'), prog[7]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(1), prog[8]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[9]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[10]);
}

SCOPE_TEST(twoFixedStrings) {
  std::vector< std::string > kws;
  kws.push_back("one");
  kws.push_back("two");
  GraphPtr fsm = createGraph(kws);
  SCOPE_ASSERT_EQUAL(7u, fsm->numVertices());
  SCOPE_ASSERT_EQUAL(3u, calculateLMin(*fsm));
}

SCOPE_TEST(twoUnicode) {
  std::vector< std::string > kws;
  kws.push_back("aa");
  kws.push_back("ab");
  GraphPtr fsm = createGraph(kws, CP_UCS16);
  SCOPE_ASSERT_EQUAL(7u, fsm->numVertices());
  SCOPE_ASSERT_EQUAL(1u, fsm->outDegree(0));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(1));
  SCOPE_ASSERT_EQUAL(1u, fsm->outDegree(1));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(2));
  SCOPE_ASSERT_EQUAL(2u, fsm->outDegree(2));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(3));
  SCOPE_ASSERT_EQUAL(1u, fsm->outDegree(3));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(4));
  SCOPE_ASSERT_EQUAL(0u, fsm->outDegree(4));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(5));
  SCOPE_ASSERT_EQUAL(1u, fsm->outDegree(5));
  SCOPE_ASSERT_EQUAL(1u, fsm->inDegree(6));
  SCOPE_ASSERT_EQUAL(0u, fsm->outDegree(6));
  SCOPE_ASSERT_EQUAL(4u, calculateLMin(*fsm));
}

SCOPE_TEST(firstBitset) {
  Graph fsm(3);
  edge(0, 1, fsm, new LitState('A'));
  edge(0, 2, fsm, new LitState('B'));

  ByteSet accepted = firstBytes(fsm);
  for (unsigned int i = 0; i < 256; ++i) {
    if (i == 'A' || i == 'B') {
      SCOPE_ASSERT(accepted[i]);
    }
    else {
      SCOPE_ASSERT(!accepted[i]);
    }
  }
}

SCOPE_TEST(simpleCollapse) {
  std::vector< std::string > kws;
  kws.push_back("ab");
  kws.push_back("ac");
  GraphPtr fsm = createGraph(kws);
  SCOPE_ASSERT_EQUAL(4u, fsm->numVertices());
  SCOPE_ASSERT_EQUAL(1u, fsm->outDegree(0));
  SCOPE_ASSERT_EQUAL(2u, fsm->outDegree(1));
  SCOPE_ASSERT_EQUAL(0u, fsm->outDegree(2));
  SCOPE_ASSERT_EQUAL(0u, fsm->outDegree(3));
  SCOPE_ASSERT_EQUAL(2u, calculateLMin(*fsm));
}

SCOPE_TEST(codeGen2DiscoverVertex) {
  Graph fsm(2);
  edge(0, 1, fsm, new LitState('a'));
  boost::shared_ptr<CodeGenHelper> cg(new CodeGenHelper(fsm.numVertices()));
  CodeGenVisitor vis(cg);

  vis.discover_vertex(1, fsm);
  SCOPE_ASSERT_EQUAL(1u, cg->NumDiscovered);
  SCOPE_ASSERT_EQUAL(0u, cg->DiscoverRanks[1]);

  vis.discover_vertex(0, fsm);
  SCOPE_ASSERT_EQUAL(2u, cg->NumDiscovered);
  SCOPE_ASSERT_EQUAL(1u, cg->DiscoverRanks[0]);
}

SCOPE_TEST(codeGen2FinishVertex) {
  Graph fsm(5);
  edge(0, 1, fsm, new LitState('a'));
  edge(1, 2, fsm, new LitState('b'));
  edge(2, 3, fsm, new LitState('c'));
  edge(2, 4, fsm, new LitState('d'));
  boost::shared_ptr<CodeGenHelper> cg(new CodeGenHelper(fsm.numVertices()));
  CodeGenVisitor vis(cg);

  cg->NumDiscovered = 3;
  cg->DiscoverRanks[0] = 0;
  cg->DiscoverRanks[1] = 1;
  cg->DiscoverRanks[2] = 2;
  cg->DiscoverRanks[3] = 3;
  cg->DiscoverRanks[4] = 4;

  vis.finish_vertex(0, fsm);
  SCOPE_ASSERT_EQUAL(0u, cg->Snippets[0].Start);
  SCOPE_ASSERT_EQUAL(0u, cg->Snippets[0].numTotal());

  vis.finish_vertex(1, fsm);
  SCOPE_ASSERT_EQUAL(0u, cg->Snippets[1].Start);
  SCOPE_ASSERT_EQUAL(1u, cg->Snippets[1].numTotal());

  vis.finish_vertex(2, fsm);
  SCOPE_ASSERT_EQUAL(1u, cg->Snippets[2].Start);
  SCOPE_ASSERT_EQUAL(3u, cg->Snippets[2].numTotal());

  vis.finish_vertex(3, fsm);
  SCOPE_ASSERT_EQUAL(4u, cg->Snippets[3].Start);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[3].numTotal());
  
  vis.finish_vertex(4, fsm);
  SCOPE_ASSERT_EQUAL(6u, cg->Snippets[4].Start);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[4].numTotal());
}

SCOPE_TEST(alternationCodeGen2FinishVertex) {
  Graph fsm(3);
  edge(0, 1, fsm, new LitState('a'));
  edge(0, 2, fsm, new LitState('b'));
  boost::shared_ptr<CodeGenHelper> cg(new CodeGenHelper(fsm.numVertices()));
  CodeGenVisitor vis(cg);
  
  cg->NumDiscovered = 3;
  cg->DiscoverRanks[0] = 0;
  cg->DiscoverRanks[1] = 1;
  cg->DiscoverRanks[2] = 2;

  vis.finish_vertex(0, fsm);
  SCOPE_ASSERT_EQUAL(0u, cg->Snippets[0].Start);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[0].numTotal());
  
  vis.finish_vertex(1, fsm);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[1].Start);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[1].numTotal());
  
  vis.finish_vertex(2, fsm);
  SCOPE_ASSERT_EQUAL(4u, cg->Snippets[2].Start);
  SCOPE_ASSERT_EQUAL(2u, cg->Snippets[2].numTotal());
}

SCOPE_TEST(layoutWithCheckHalt) {
  Graph fsm(3);
  edge(0, 1, fsm, new LitState('a'));
  edge(1, 2, fsm, new LitState('b'));
  edge(2, 2, fsm, new LitState('b', 0));
  
  boost::shared_ptr<CodeGenHelper> cg(new CodeGenHelper(fsm.numVertices()));
  CodeGenVisitor vis(cg);
  specialVisit(fsm, 0, vis);

  SCOPE_ASSERT_EQUAL(0u, cg->DiscoverRanks[0]);
  SCOPE_ASSERT_EQUAL(1u, cg->DiscoverRanks[1]);
  SCOPE_ASSERT_EQUAL(2u, cg->DiscoverRanks[2]);
  SCOPE_ASSERT_EQUAL(StateLayoutInfo(0u, 0u, 0u, UNALLOCATED), cg->Snippets[0]);
  SCOPE_ASSERT_EQUAL(StateLayoutInfo(0u, 1u, 0u, UNALLOCATED), cg->Snippets[1]);
  SCOPE_ASSERT_EQUAL(StateLayoutInfo(1u, 1u, 5u, 1u), cg->Snippets[2]);
}

SCOPE_TEST(twoStateBetterLayout) {
  Graph fsm(2);
  edge(0, 1, fsm, new LitState('a'));
  
  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  SCOPE_ASSERT_EQUAL(2u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[1]);
}

SCOPE_TEST(alternationBetterLayout) {
  Graph fsm(3);
  Transition* t1(new LitState('a', 0)),
            * t2(new LitState('b', 0));
  edge(0, 1, fsm, t1);
  edge(0, 2, fsm, t2);
  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  
  // std::cout << "alternationBetterLayout" << '\n' << prog;
  SCOPE_ASSERT_EQUAL(10u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLongFork(&prog[0], 6), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[2]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[3]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[4]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[5]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[6]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[7]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[8]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[9]);  
}

void createTrie(Graph& fsm) {
  edge(0, 1, fsm, new LitState('a'));
  edge(1, 2, fsm, new LitState('b'));
  edge(2, 3, fsm, new LitState('l'));
  edge(3, 4, fsm, new LitState('e', 0));
  edge(2, 5, fsm, new LitState('e'));
  edge(5, 6, fsm, new LitState('t', 1));
  edge(0, 7, fsm, new LitState('b'));
  edge(7, 8, fsm, new LitState('i'));
  edge(8, 9, fsm, new LitState('t'));
  edge(9, 10, fsm, new LitState('e', 2));
}

SCOPE_TEST(betterLayout) {
  // a
  //  b
  //   le
  //   et
  // b
  //  ite

  Graph fsm(11);
  createTrie(fsm);

  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);

  // std::cout << "betterLayout\n" << prog;
  SCOPE_ASSERT_EQUAL(25u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLongFork(&prog[0], 8), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[2]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[3]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongFork(&prog[4], 15), prog[4]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[6], 20), prog[6]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[8]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('i'), prog[9]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('t'), prog[10]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('e'), prog[11]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(2), prog[12]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[13]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[14]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('l'), prog[15]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('e'), prog[16]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[17]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[18]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[19]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('e'), prog[20]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('t'), prog[21]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(1), prog[22]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[23]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[24]);
}

SCOPE_TEST(generateCheckHalt) {
  Graph fsm(2);
  edge(0, 1, fsm, new LitState('a', 0));
  edge(1, 1, fsm, new LitState('a', 0));

  fsm[1]->Label = 0;
  fsm[1]->IsMatch = true;

  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  SCOPE_ASSERT_EQUAL(1u, prog.NumChecked);

  // std::cout << prog;
  SCOPE_ASSERT_EQUAL(6u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeCheckHalt(1), prog[1]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[2]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[3]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[4], 0), prog[4]);
}

SCOPE_TEST(testInitVM) {
  std::vector<std::string> keys;
  SearchInfo info;
  keys.push_back("one");
  keys.push_back("two");
  boost::shared_ptr<VmInterface> search = initVM(keys, info);
               //012345678901234
  byte text[] = "a onetwothree";
  MockCallback cb;
  SCOPE_ASSERT(!search->search(&text[0], &text[15], 0, cb));
  search->closeOut(cb);
  SCOPE_ASSERT_EQUAL(2u, cb.Hits.size());
  SCOPE_ASSERT_EQUAL(SearchHit(2, 3, 0), cb.Hits[0]);
  SCOPE_ASSERT_EQUAL(SearchHit(5, 3, 1), cb.Hits[1]);
}

SCOPE_TEST(testPivotTransitions) {
  Graph fsm(5);
  edge(0, 1, fsm, new LitState('a', 0));
  edge(0, 2, fsm, new LitState('a', 1));
  edge(0, 3, fsm, new LitState('z'));
  edge(0, 4, fsm, new LitState('z'));
  std::vector< std::vector< Graph::vertex > > tbl = pivotStates(0, fsm);
  SCOPE_ASSERT_EQUAL(256u, tbl.size());
  for (uint32 i = 0; i < 256; ++i) {
    if (i == 'a') {
      SCOPE_ASSERT_EQUAL(2u, tbl[i].size());
      SCOPE_ASSERT(std::find(tbl[i].begin(), tbl[i].end(), 1) != tbl[i].end());
      SCOPE_ASSERT(std::find(tbl[i].begin(), tbl[i].end(), 2) != tbl[i].end());
    }
    else if (i == 'z') {
      SCOPE_ASSERT_EQUAL(2u, tbl[i].size());
      SCOPE_ASSERT(std::find(tbl[i].begin(), tbl[i].end(), 3) != tbl[i].end());
      SCOPE_ASSERT(std::find(tbl[i].begin(), tbl[i].end(), 4) != tbl[i].end());
    }
    else {
      SCOPE_ASSERT(tbl[i].empty());
    }
  }
}

SCOPE_TEST(testBitVectorGeneration) {
  ByteSet    bits;
  bits.reset();
  bits.set('0');
  bits.set('2');
  bits.set('4');
  bits.set('8');
  Graph fsm(2);
  edge(0, 1, fsm, new CharClassState(bits, "0248"));
  fsm[1]->Label = 0;
  fsm[1]->IsMatch = true;

  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  SCOPE_ASSERT_EQUAL(12u, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeBitVector(), prog[0]);
  SCOPE_ASSERT_EQUAL(bits, *reinterpret_cast<ByteSet*>(&prog[1]));
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[9]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[10]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[11]);
}

SCOPE_TEST(testMaxOutbound) {
  Graph fsm(5);
  edge(0, 1, fsm, new LitState('a'));
  edge(0, 2, fsm, new LitState('a'));
  edge(0, 3, fsm, new LitState('b'));
  edge(0, 4, fsm, new LitState('c'));
  std::vector< std::vector< Graph::vertex > > tbl = pivotStates(0, fsm);
  SCOPE_ASSERT_EQUAL(2u, maxOutbound(tbl));
}

SCOPE_TEST(generateJumpTableRange) {
  Graph fsm(7); // a(b|c|d|g)f
  edge(0, 1, fsm, new LitState('a'));
  edge(1, 2, fsm, new LitState('b'));
  edge(1, 3, fsm, new LitState('c'));
  edge(1, 4, fsm, new LitState('d'));
  edge(1, 5, fsm, new LitState('g'));
  edge(2, 6, fsm, new LitState('f'));
  edge(3, 6, fsm, new LitState('f'));
  edge(4, 6, fsm, new LitState('f'));
  edge(5, 6, fsm, new LitState('f', 0));

  ProgramPtr p = createProgram(fsm);
  Program& prog(*p);
  SCOPE_ASSERT_EQUAL(23, prog.size());
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('a'), prog[0]);
  SCOPE_ASSERT_EQUAL(Instruction::makeJumpTableRange('b', 'g'), prog[1]);
  SCOPE_ASSERT_EQUAL(9, *(uint32*) &prog[2]); // b
  SCOPE_ASSERT_EQUAL(9, *(uint32*) &prog[3]); // c
  SCOPE_ASSERT_EQUAL(9, *(uint32*) &prog[4]); // d
  SCOPE_ASSERT_EQUAL(0xffffffff, *(uint32*) &prog[5]); // e
  SCOPE_ASSERT_EQUAL(0xffffffff, *(uint32*) &prog[6]); // f
  SCOPE_ASSERT_EQUAL(9, *(uint32*) &prog[7]); // g
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('b'), prog[8]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('f'), prog[9]);
  SCOPE_ASSERT_EQUAL(Instruction::makeCheckHalt(1), prog[10]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLabel(0), prog[11]);
  SCOPE_ASSERT_EQUAL(Instruction::makeMatch(), prog[12]);
  SCOPE_ASSERT_EQUAL(Instruction::makeHalt(), prog[13]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('c'), prog[14]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[15], 9), prog[15]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('d'), prog[17]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[18], 9), prog[18]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLit('g'), prog[20]);
  SCOPE_ASSERT_EQUAL(Instruction::makeLongJump(&prog[21], 9), prog[21]);
}

SCOPE_TEST(testCreateXXYYY) {
  std::vector<std::string> kws;
  kws.push_back("x");
  kws.push_back("x");
  kws.push_back("yyy");

  GraphPtr gp(createGraph(kws, CP_ASCII, true, false));
  Graph& g = *gp;

  Graph exp;
  edge(0, 1, exp, new LitState('x', 0));  
  edge(0, 2, exp, new LitState('x', 1));
  edge(0, 3, exp, new LitState('y'));
  edge(3, 4, exp, new LitState('y'));
  edge(4, 5, exp, new LitState('y', 2));
  
  exp[1]->Label = 0;
  exp[2]->Label = 1;
  exp[3]->Label = 2;
  exp[4]->Label = UNALLOCATED; 
  exp[5]->Label = UNALLOCATED;

  exp[1]->IsMatch = true;
  exp[2]->IsMatch = true;
  exp[5]->IsMatch = true;

  ASSERT_EQUAL_GRAPHS(exp, g);
  ASSERT_EQUAL_LABELS(exp, g);
  ASSERT_EQUAL_MATCHES(exp, g);  
}
