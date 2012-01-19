#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

#include <boost/bind.hpp>

template <class GraphData, class VertexData, class EdgeData>
class Graph : public GraphData
{
private:
  struct Vertex;
  struct Edge;

  typedef typename std::vector<Vertex> VList;
  typedef typename std::vector<Edge> EList;

public:
  typedef typename VList::size_type VertexDescriptor;
  typedef typename EList::size_type EdgeDescriptor;

  typedef typename VList::size_type VertexSizeType;
  typedef typename EList::size_type EdgeSizeType;

private:
  typedef typename std::vector<EdgeDescriptor> EDList;

  struct Vertex : public VertexData {
    Vertex(): VertexData() {}
    Vertex(const VertexData& vdata): VertexData(vdata) {}

    EDList In, Out;
  };

  struct Edge : public EdgeData {
    Edge(VertexDescriptor head, VertexDescriptor tail, const EdgeData& edata): EdgeData(edata), Head(head), Tail(tail) {}

    VertexDescriptor Head, Tail;
  };

public:

  Graph(VertexSizeType vActual = 0): Vertices(vActual, Vertex()) {}

  Graph(VertexSizeType vActual, VertexSizeType vReserve, EdgeSizeType eReserve = 0) {
    if (vReserve > vActual) {
      Vertices.reserve(vReserve);
    }

    Vertices.resize(vActual, Vertex());

    if (eReserve > 0) {
      Edges.reserve(eReserve);
    }
  }

  //
  // lookup & access
  //

  VertexDescriptor inVertex(VertexDescriptor tail, EdgeSizeType i) const {
    return Edges[Vertices[tail].In[i]].Head;
  }

  VertexDescriptor outVertex(VertexDescriptor head, EdgeSizeType i) const {
    return Edges[Vertices[head].Out[i]].Tail;
  }

  EdgeDescriptor inEdge(VertexDescriptor tail, EdgeSizeType i) const {
    return Vertices[tail].In[i];
  }

  EdgeDescriptor outEdge(VertexDescriptor head, EdgeSizeType i) const {
    return Vertices[head].Out[i];
  }

  EdgeSizeType inDegree(VertexDescriptor tail) const {
    return Vertices[tail].In.size();
  }

  EdgeSizeType outDegree(VertexDescriptor head) const {
    return Vertices[head].Out.size();
  }

  VertexData& operator[](VertexDescriptor vd) {
    return Vertices[vd];
  }

  const VertexData& operator[](VertexDescriptor vd) const {
    return Vertices[vd];
  }

  EdgeData& operator()(EdgeDescriptor ed) {
    return Edges[ed];
  }

  const EdgeData& operator()(EdgeDescriptor ed) const {
    return Edges[ed];
  }

  //
  // modifiers
  //

  void clear() {
    clearVertices();
  }

  void clearEdges() {
    Edges.clear();

    const typename VList::iterator iend(Vertices.end());
    for (typename VList::iterator i(Vertices.begin()); i != iend; ++i) {
      i->In.clear();
      i->Out.clear();
    }
  }

  void clearVertices() {
    Vertices.clear();
    Edges.clear();
  }

  VertexDescriptor addVertex(const VertexData& vdata = VertexData()) {
    Vertices.push_back(Vertex(vdata));
    return Vertices.size() - 1;
  }

  void removeVertex(VertexDescriptor vd) {
    const Vertex& v(Vertices[vd]);
    std::for_each(v.In.begin(), v.In.end(), boost::bind(&Graph::removeEdge, this, _1));
    std::for_each(v.Out.begin(), v.Out.end(), boost::bind(&Graph::removeEdge, this, _1));

    compact_vertex(vd);
  }

  EdgeDescriptor addEdge(VertexDescriptor head, VertexDescriptor tail, const EdgeData& edata = EdgeData()) {
    Edges.push_back(Edge(head, tail, edata));
    const EdgeDescriptor ed = Edges.size() - 1;
    Vertices[head].Out.push_back(ed);
    Vertices[tail].In.push_back(ed);
    return ed;
  }

  EdgeDescriptor insertEdge(VertexDescriptor head, VertexDescriptor tail, EdgeSizeType hi = std::numeric_limits<EdgeSizeType>::max(), EdgeSizeType ti = std::numeric_limits<EdgeSizeType>::max(), const EdgeData& edata = EdgeData()) {
    Edges.push_back(Edge(head, tail, edata));
    const EdgeDescriptor ed = Edges.size() - 1;

    attach_edge(Vertices[head].Out, hi, ed);
    attach_edge(Vertices[tail].In, ti, ed);

    return ed;
  }

  void removeEdge(EdgeDescriptor ed) {
    detach_edge(Vertices[Edges[ed].Head].Out, ed);
    detach_edge(Vertices[Edges[ed].Tail].In, ed);

    compact_edge(ed);
  }

  //
  // capacity
  //

  VertexSizeType verticesSize() const {
    return Vertices.size();
  }

  EdgeSizeType edgesSize() const {
    return Edges.size();
  }

  VertexSizeType verticesCapacity() const {
    return Vertices.capacity();
  }

  EdgeSizeType edgesCapacity() const {
    return Edges.capacity();
  }

  void reserveVertices(VertexSizeType size) {
    return Vertices.reserve(size);
  }

  void reserveEdges(EdgeSizeType size) {
    return Edges.reserve(size);
  }

private:
  void attach_edge(EDList& elist, EdgeSizeType epos, EdgeDescriptor ed) {
    elist.insert(epos < elist.size() ? elist.begin() + epos : elist.end(), ed);
  }

  void detach_edge(EDList& elist, EdgeDescriptor ed) {
    elist.erase(std::find(elist.begin(), elist.end(), ed));
  }

  void compact_edge(EdgeDescriptor ed) {
    typename EList::iterator i(Edges.erase(Edges.begin() + ed));
    const typename EList::iterator iend(Edges.end());
    for ( ; i != iend; ++i) {
      EDList& out(Vertices[i->Head].Out);
      --*std::find(out.begin(), out.end(), i - Edges.begin() + 1);

      EDList& in(Vertices[i->Tail].In);
      --*std::find(in.begin(), in.end(), i - Edges.begin() + 1);
    }
  }

  void compact_vertex(VertexDescriptor vd) {
    Vertices.erase(Vertices.begin() + vd);

    const typename EList::iterator iend(Edges.end());
    for (typename EList::iterator i(Edges.begin()); i != iend; ++i) {
      if (i->Head > vd) {
        --i->Head;
      }

      if (i->Tail > vd) {
        --i->Tail;
      }
    }
  }

  VList Vertices;
  EList Edges;
};

template <class G, class V, class E> std::ostream& operator<<(std::ostream& out, const Graph<G,V,E>& g) {
  const typename Graph<G,V,E>::VertexSizeType vnum = g.verticesSize();

  // print graph size
  out << "|g| = " << vnum << '\n';

  // print out edges for each vertex
  for (typename Graph<G,V,E>::VertexSizeType v = 0; v < vnum; ++v) {
    const typename Graph<G,V,E>::EdgeSizeType odeg = g.outDegree(v);
    for (typename Graph<G,V,E>::EdgeSizeType o = 0; o < odeg; ++o) {
      out << v << " -> " << g.outVertex(v, o) << '\n';
    }
  }

  return out;
}
