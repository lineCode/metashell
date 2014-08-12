
// Metashell - Interactive C++ template metaprogramming shell
// Copyright (C) 2014, Andras Kucsma (andras.kucsma@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <map>
#include <deque>
#include <utility>
#include <iostream>
#include <cassert>
#include <algorithm>

#include <boost/regex.hpp>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/graphviz.hpp>

#include <metashell/templight_trace.hpp>
#include <metashell/metadebugger_shell.hpp>

namespace metashell {

const std::vector<just::console::color> templight_trace::colors =
  boost::assign::list_of
    (just::console::color::red)
    (just::console::color::green)
    (just::console::color::yellow)
    (just::console::color::blue)
    (just::console::color::magenta)
    (just::console::color::cyan);


templight_trace::vertex_descriptor templight_trace::add_vertex(
  const std::string& element,
  const file_location& point_of_instantiation)
{
  element_vertex_map_t::iterator pos;
  bool inserted;

  boost::tie(pos, inserted) = element_vertex_map.insert(
      std::make_pair(element, vertex_descriptor()));

  if (inserted) {
    vertex_descriptor vertex = boost::add_vertex(graph);
    pos->second = vertex;

    template_vertex_property& vertex_property =
      boost::get(template_vertex_property_tag(), graph, vertex);

    vertex_property.name = element;
    vertex_property.point_of_instantiation = point_of_instantiation;

    return vertex;
  }
  return pos->second;
}

void templight_trace::add_edge(
    vertex_descriptor from,
    vertex_descriptor to,
    instantiation_kind kind)
{
  edge_descriptor edge;
  bool inserted;

  boost::tie(edge, inserted) = boost::add_edge(from, to, graph);

  assert(inserted);

  boost::get(template_edge_property_tag(), graph, edge).kind = kind;
}

boost::optional<templight_trace::vertex_descriptor>
  templight_trace::find_vertex(const std::string& element) const
{
  element_vertex_map_t::const_iterator it =
    element_vertex_map.find(element);

  if (it == element_vertex_map.end()) {
    return boost::none;
  }

  return it->second;
}

void templight_trace::print_graph(std::ostream& os) const {

  const_vertex_property_map_t vertex_map =
      boost::get(template_vertex_property_tag(), graph);

  const_edge_property_map_t edge_map =
      boost::get(template_edge_property_tag(), graph);

  os << "Verticies:\n";
  BOOST_FOREACH(vertex_descriptor vertex, boost::vertices(graph)) {
    const template_vertex_property& vertex_property =
      boost::get(vertex_map, vertex);

    os << vertex << " : " <<
      vertex_property.name <<
      " instantiated from " <<
      vertex_property.point_of_instantiation << '\n';
  }

  os << "Edges:\n";
  BOOST_FOREACH(const edge_descriptor& edge, boost::edges(graph)) {
    os << boost::get(vertex_map, source(edge, graph)).name <<
      " ---" << boost::get(edge_map, edge).kind << "---> " <<
      boost::get(vertex_map, target(edge, graph)).name << '\n';
  }
}

void templight_trace::print_graphviz(std::ostream& os) const {

  boost::write_graphviz(
      os, graph,
      [this](std::ostream& os, vertex_descriptor vertex) {
        os << "[label=\"" <<
          boost::get(template_vertex_property_tag(), graph, vertex).name <<
          "\"]";
      },
      [this](std::ostream& os, edge_descriptor edge) {
        os << "[label=\""<<
          boost::get(template_edge_property_tag(), graph, edge).kind <<
          "\"]";
      }
  );
}

templight_trace::string_range templight_trace::find_type_emphasize(
    const std::string& type) const
{

  #define MSH_R_NAMESPACE_OR_TYPE       \
    "("                                 \
      "([_a-zA-Z][_a-zA-Z0-9]*)"    "|" \
      "(\\(anonymous namespace\\))" "|" \
      "(<anonymous>)"               "|" \
      "(<anonymous struct>)"        "|" \
      "(<anonymous class>)"         "|" \
      "(<anonymous union>)"             \
    ")"

  boost::regex reg(
      "^(::)?(" MSH_R_NAMESPACE_OR_TYPE "::)*" MSH_R_NAMESPACE_OR_TYPE);

  #undef MSH_R_NAMESPACE_OR_TYPE

  boost::smatch match;
  if (!boost::regex_search(type.begin(), type.end(), match, reg)) {
    return string_range(type.end(), type.end());
  }

  return string_range(match[10].first, match[10].second);
}

void templight_trace::print_trace_graph(
    const metadebugger_shell& sh,
    unsigned depth,
    const std::vector<unsigned>& depth_counter,
    bool print_mark) const
{
  assert(depth_counter.size() > depth);

  if (depth > 0) {
    //TODO respect the -H (no syntax highlight parameter)
    for (unsigned i = 1; i < depth; ++i) {
      sh.display(
          depth_counter[i] > 0 ? "| " : "  ",
          colors[i % colors.size()]);
    }

    just::console::color mark_color = colors[depth % colors.size()];
    if (print_mark) {
      sh.display("+ ", mark_color);
    } else if (depth_counter[depth] > 0) {
      sh.display("| ", mark_color);
    } else {
      sh.display("  ");
    }
  }
}

namespace {

void print_range(
    const metadebugger_shell& sh,
    std::string::const_iterator begin,
    std::string::const_iterator end,
    metadebugger_shell::optional_color c)
{
  if (begin < end) {
    sh.display(std::string(begin, end), c);
  }
}

}

void templight_trace::print_trace_content(
    const metadebugger_shell& sh,
    string_range range,
    string_range emphasize) const
{
  assert(range.first <= range.second);
  assert(emphasize.first <= emphasize.second);

  //TODO avoid copying

  print_range(
      sh,
      range.first,
      std::min(range.second, emphasize.first),
      boost::none);

  print_range(
      sh,
      std::max(range.first, emphasize.first),
      std::min(range.second, emphasize.second),
      just::console::color::white);

  print_range(
      sh,
      std::max(emphasize.second, range.first),
      range.second,
      boost::none);
}

void templight_trace::print_trace_line(
    const metadebugger_shell& sh,
    vertex_descriptor vertex,
    unsigned depth,
    const std::vector<unsigned>& depth_counter,
    const boost::optional<instantiation_kind>& kind,
    unsigned width) const
{

  const std::string type =
    boost::get(template_vertex_property_tag(), graph, vertex).name;

  std::stringstream element_content_ss;
  element_content_ss << type;

  if (kind) {
    element_content_ss << " (" << *kind << ")";
  }

  std::string element_content = element_content_ss.str();

  string_range emphasize = find_type_emphasize(type);

  // Realign the iterators from 'type' to 'element_content'
  emphasize.first = element_content.begin() + (emphasize.first - type.begin());
  emphasize.second = element_content.begin() + (emphasize.second - type.begin());

  unsigned non_content_length = 2*depth;

  if (width < 10 || non_content_length >= width - 10) {
    // We have no chance to display the graph nicely :(
    print_trace_graph(sh, depth, depth_counter, true);

    print_trace_content(
      sh,
      string_range(element_content.begin(), element_content.end()),
      emphasize);
    sh.display("\n");
  } else {
    unsigned content_width = width - non_content_length;
    for (unsigned i = 0; i < element_content.size(); i += content_width) {
      print_trace_graph(sh, depth, depth_counter, i == 0);
      print_trace_content(
        sh,
        string_range(
          element_content.begin() + i,
          i + content_width < element_content.size() ?
            element_content.begin() + (i + content_width) :
            element_content.end()
        ),
        emphasize
      );
      sh.display("\n");
    }
  }
}

// Visits a single vertex and all of its children
template<class EdgeIterator, class GetEdges, class EdgeDirection>
void templight_trace::print_trace_visit(
    const metadebugger_shell& sh,
    vertex_descriptor root_vertex,
    discovered_t& discovered,
    GetEdges get_edges, EdgeDirection edge_direction,
    unsigned width) const
{

  assert(discovered.size() == boost::num_vertices(graph));

  if (discovered[root_vertex]) {
    return;
  }
  // -----
  // Customized DFS
  //   The algorithm only checks vertices which are reachable from root_vertex
  // ----

  // This vector counts how many elements are in the to_visit
  // stack for each specific depth.
  // The purpose is to not draw pipes, when a tree element
  // doesn't have any more children.
  // The 0th element is never read.
  std::vector<unsigned> depth_counter(1);

  typedef boost::tuple<
    vertex_descriptor,
    unsigned, // Depth
    boost::optional<instantiation_kind> > stack_element;

  // The usual stack for DFS
  std::stack<stack_element> to_visit;

  // We don't care about the instantiation_kind for the source vertex
  to_visit.push(boost::make_tuple(root_vertex, 0, boost::none));
  ++depth_counter[0]; // This value is neved read

  while (!to_visit.empty()) {
    unsigned depth;
    vertex_descriptor vertex;
    boost::optional<instantiation_kind> kind;
    boost::tie(vertex, depth, kind) = to_visit.top();
    to_visit.pop();

    --depth_counter[depth];

    print_trace_line(sh, vertex, depth, depth_counter, kind, width);

    if (!discovered[vertex]) {
      discovered[vertex] = true;

      EdgeIterator begin, end;
      boost::tie(begin, end) = get_edges(vertex);

      typedef std::vector<edge_descriptor> edges_t;
      edges_t edges(begin, end);

      if (depth_counter.size() <= depth+1) {
        depth_counter.resize(depth+1+1);
      }

      // Reverse iteration, so types that got instantiated first
      // get on the top of the stack
      BOOST_REVERSE_FOREACH(const edge_descriptor& edge, edges) {
        instantiation_kind next_kind =
          boost::get(template_edge_property_tag(), graph, edge).kind;

        to_visit.push(
          boost::make_tuple(edge_direction(edge), depth+1, next_kind));

        ++depth_counter[depth+1];
      }
    }
  }
}

void templight_trace::print_forwardtrace(
    const metadebugger_shell& sh,
    const std::string& type) const
{

  boost::optional<vertex_descriptor> opt_vertex =
    find_vertex(type);

  if (!opt_vertex) {
    sh.display("type \"" + type + "\" not found", just::console::color::red);
    return;
  }

  unsigned width = sh.width();

  discovered_t discovered(boost::num_vertices(graph));

  print_trace_visit<out_edge_iterator>(
      sh,
      *opt_vertex,
      discovered,
      [this](vertex_descriptor v) { return boost::out_edges(v, graph); },
      [this](edge_descriptor e) { return boost::target(e, graph); },
      width);
}

void templight_trace::print_full_forwardtrace(const metadebugger_shell& sh) const {

  assert(boost::num_vertices(graph) > 0);

  unsigned width = sh.width();

  discovered_t discovered(boost::num_vertices(graph));

  print_trace_visit<out_edge_iterator>(
      sh,
      // 0 is always the <root> vertex, and every vertex is reachable from root
      0,
      discovered,
      [this](vertex_descriptor v) { return boost::out_edges(v, graph); },
      [this](edge_descriptor e) { return boost::target(e, graph); },
      width);
}

void templight_trace::print_backtrace(
    const metadebugger_shell& sh,
    const std::string& type) const
{

  boost::optional<vertex_descriptor> opt_vertex =
    find_vertex(type);

  if (!opt_vertex) {
    sh.display("type \"" + type + "\" not found", just::console::color::red);
    return;
  }

  unsigned width = sh.width();

  discovered_t discovered(boost::num_vertices(graph));

  print_trace_visit<in_edge_iterator>(
      sh,
      *opt_vertex,
      discovered,
      [this](vertex_descriptor v) { return boost::in_edges(v, graph); },
      [this](edge_descriptor e) { return boost::source(e, graph); },
      width);
}

struct templight_trace::only_has_discovered_out_edge_predicate {
  only_has_discovered_out_edge_predicate(
    const graph_t& graph,
    const discovered_t& discovered) :
      graph(graph), discovered(discovered) {}

  bool operator()(vertex_descriptor vertex) const {
    if (discovered[vertex]) {
      return false;
    }
    BOOST_FOREACH(edge_descriptor edge, boost::out_edges(vertex, graph)) {
      if (!discovered[boost::target(edge, graph)]) {
        return false;
      }
    }
    return true;
  }
private:
  const graph_t& graph;
  const discovered_t& discovered;
};

void templight_trace::print_full_backtrace(const metadebugger_shell& sh) const {

  assert(boost::num_vertices(graph) > 0);

  discovered_t discovered(boost::num_vertices(graph));

  unsigned width = sh.width();

  // TODO this needs some more work:
  // -try to go with the deepest route first
  // -try to find a natural way to produce bt
  // -o(V^2) algorithm
  while (true) {
    // Since the graph is DAG, there is always a vertex which
    // has only discovered out_edges
    vertex_iterator begin, end, it;
    boost::tie(begin, end) = boost::vertices(graph);

    it = std::find_if(begin, end,
        only_has_discovered_out_edge_predicate(graph, discovered));

    if (it == end) {
      break;
    }
    print_trace_visit<in_edge_iterator>(
        sh,
        *it,
        discovered,
        [this](vertex_descriptor v) { return boost::in_edges(v, graph); },
        [this](edge_descriptor e) { return boost::source(e, graph); },
        width);
  }
}

void templight_trace::print_current_frame(const metadebugger_shell& sh) const {
  if (mp_state.vertex_stack.empty()) {
    sh.display("Stack is empty\n", just::console::color::red);
    return;
  }
  vertex_descriptor current_vertex;
  boost::optional<instantiation_kind> kind;
  boost::tie(current_vertex, kind) = mp_state.vertex_stack.top();

  sh.display(boost::get(
      template_vertex_property_tag(),
      graph,
      current_vertex).name + "\n");
}

void templight_trace::reset_metaprogram_state() {
  mp_state = metaprogram_state(*this);
}

bool templight_trace::step_metaprogram() {
  if (mp_state.vertex_stack.empty()) {
    return false;
  }

  using boost::tuples::ignore;

  vertex_descriptor current_vertex;
  boost::tie(current_vertex, ignore) = mp_state.vertex_stack.top();
  mp_state.vertex_stack.pop();

  if (!mp_state.discovered[current_vertex]) {
    mp_state.discovered[current_vertex] = true;

    for (edge_descriptor edge :
        boost::make_iterator_range(
          boost::out_edges(current_vertex, graph)))
    {
      instantiation_kind next_kind =
        boost::get(template_edge_property_tag(), graph, edge).kind;

      mp_state.vertex_stack.push(
        boost::make_tuple(
          boost::target(edge, graph), next_kind));
    }
  }
  return true;
}

templight_trace::metaprogram_state::metaprogram_state() {}

templight_trace::metaprogram_state::metaprogram_state(
    const templight_trace& trace)
{
  unsigned vertex_count = boost::num_vertices(trace.graph);
  if (vertex_count > 0) {
    discovered.resize(vertex_count, false);
    // 0 == <root> vertex
    vertex_stack.push(boost::make_tuple(0, boost::none));  }
}

std::ostream& operator<<(std::ostream& os, instantiation_kind kind) {
  switch (kind) {
    default:
        os << "UnknownKind"; break;
    case template_instantiation:
        os << "TemplateInstantiation"; break;
    case default_template_argument_instantiation:
        os << "DefaultTemplateArgumentInstantiation"; break;
    case default_function_argument_instantiation:
        os << "DefaultFunctionArgumentInstantiation"; break;
    case explicit_template_argument_substitution:
        os << "ExplicitTemplateArgumentSubstitution"; break;
    case deduced_template_argument_substitution:
        os << "DeducedTemplateArgumentSubstitution"; break;
    case prior_template_argument_substitution:
        os << "PriorTemplateArgumentSubstitution"; break;
    case default_template_argument_checking:
        os << "DefaultTemplateArgumentChecking"; break;
    case exception_spec_instantiation:
        os << "ExceptionSpecInstantiation"; break;
    case memoization:
        os << "Memoization"; break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const templight_trace& trace) {
  trace.print_graph(os);
  return os;
}

}

