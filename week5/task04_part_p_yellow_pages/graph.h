#pragma once

#include <vector>

#include "transport_catalog.pb.h"

#include "utils.h"

namespace Graph {

    using VertexId = size_t;
    using EdgeId = size_t;

    template<typename Weight>
    struct Edge {
        VertexId from;
        VertexId to;
        Weight weight;
    };

    template<typename Weight>
    class DirectedWeightedGraph {
    private:
        using IncidenceList = std::vector<EdgeId>;
        using IncidentEdgesRange = Range<typename IncidenceList::const_iterator>;

    public:
        DirectedWeightedGraph(size_t vertex_count = 0);

        DirectedWeightedGraph(const Serialization::BusGraph& serialization_graph);

        EdgeId AddEdge(const Edge<Weight> &edge);

        size_t GetVertexCount() const;

        size_t GetEdgeCount() const;

        const Edge<Weight> &GetEdge(EdgeId edge_id) const;

        IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

        Serialization::BusGraph SerializeBusGraph() const;

    private:
        std::vector<Edge<Weight>> edges_;
        std::vector<IncidenceList> incidence_lists_;
    };


    template<typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count) : incidence_lists_(vertex_count) {}

    template<typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(const Serialization::BusGraph& serialization_graph) {
        edges_.reserve(serialization_graph.edges_size());
        for (int edge_idx = 0; edge_idx < serialization_graph.edges_size(); ++edge_idx) {
            edges_.push_back({serialization_graph.edges(edge_idx).from(),
                              serialization_graph.edges(edge_idx).to(),
                              serialization_graph.edges(edge_idx).weight()});

        }

        incidence_lists_.reserve(serialization_graph.incidence_lists_size());
        for (int incidence_list_idx = 0; incidence_list_idx < serialization_graph.incidence_lists_size(); ++incidence_list_idx) {
            const Serialization::IncidenceList& serialization_cur_incidence_list = serialization_graph.incidence_lists(incidence_list_idx);

            IncidenceList cur_incidence_list;
            cur_incidence_list.reserve(serialization_cur_incidence_list.incident_vertexes_size());
            for (int incident_vertex_idx = 0; incident_vertex_idx < serialization_cur_incidence_list.incident_vertexes_size(); ++incident_vertex_idx) {
                cur_incidence_list.push_back(serialization_cur_incidence_list.incident_vertexes(incident_vertex_idx));
            }

            incidence_lists_.push_back(cur_incidence_list);
        }
    }

    template<typename Weight>
    EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight> &edge) {
        edges_.push_back(edge);
        const EdgeId id = edges_.size() - 1;
        incidence_lists_[edge.from].push_back(id);
        return id;
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists_.size();
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges_.size();
    }

    template<typename Weight>
    const Edge<Weight> &DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
        return edges_[edge_id];
    }

    template<typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
    DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
        const auto &edges = incidence_lists_[vertex];
        return {std::begin(edges), std::end(edges)};
    }

    template<typename Weight>
    Serialization::BusGraph DirectedWeightedGraph<Weight>::SerializeBusGraph() const {
        Serialization::BusGraph serialization_graph;

        for (const auto &cur_edge : edges_) {
            Serialization::GraphEdge serialization_graph_edge;
            serialization_graph_edge.set_from(cur_edge.from);
            serialization_graph_edge.set_to(cur_edge.to);
            serialization_graph_edge.set_weight(cur_edge.weight);

            *serialization_graph.add_edges() = serialization_graph_edge;
        }

        for (const auto &cur_incidence_list : incidence_lists_) {
            Serialization::IncidenceList serialization_incidence_list;
            for (const auto cur_incdent_vertex: cur_incidence_list) {
                serialization_incidence_list.add_incident_vertexes(cur_incdent_vertex);
            }
            *serialization_graph.add_incidence_lists() = serialization_incidence_list;
        }

        return serialization_graph;
    }
}
