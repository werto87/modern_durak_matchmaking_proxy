#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mem_fn.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

struct node_base
{
  node_base () : m_next (0) {}

  // Each node manages all of its tail nodes
  virtual ~node_base () { delete m_next; }

  // Access the rest of the list
  node_base *
  next () const
  {
    return m_next;
  }

  // print to the stream
  virtual void print (std::ostream &s) const = 0;

  // double the value
  virtual void double_me () = 0;

  void
  append (node_base *p)
  {
    if (m_next) m_next->append (p);
    else
      m_next = p;
  }

private:
  node_base *m_next;
};

inline std::ostream &
operator<< (std::ostream &s, node_base const &n)
{
  n.print (s);
  return s;
}

template <class T> struct node : node_base
{
  node (T x) : m_value (x) {}

  void
  print (std::ostream &s) const
  {
    s << this->m_value;
  }
  void
  double_me ()
  {
    m_value += m_value;
  }

private:
  T m_value;
};

template <class Value> class node_iter : public boost::iterator_facade<node_iter<Value>, Value, boost::forward_traversal_tag>
{
public:
  node_iter () : m_node (0) {}

  explicit node_iter (Value *p) : m_node (p) {}

private:
  struct enabler
  {
  };

public:
  template <class OtherValue> node_iter (node_iter<OtherValue> const &other, typename boost::enable_if<boost::is_convertible<OtherValue *, Value *>, enabler>::type = enabler ()) : m_node (other.m_node) {}

private:
  friend class boost::iterator_core_access;
  template <class> friend class node_iter;

  template <class OtherValue>
  bool
  equal (node_iter<OtherValue> const &other) const
  {
    return this->m_node == other.m_node;
  }

  void
  increment ()
  {
    m_node = m_node->next ();
  }

  Value &
  dereference () const
  {
    return *m_node;
  }

  Value *m_node;
};
typedef node_iter<node_base> node_iterator;
typedef node_iter<node_base const> node_const_iterator;

int
main ()
{
  std::unique_ptr<node<int> > nodes (new node<int> (42));
  nodes->append (new node<std::string> (" is greater than "));
  nodes->append (new node<int> (13));

  // Check interoperability
  assert (node_iterator (nodes.get ()) == node_const_iterator (nodes.get ()));
  assert (node_const_iterator (nodes.get ()) == node_iterator (nodes.get ()));

  assert (node_iterator (nodes.get ()) != node_const_iterator ());
  assert (node_const_iterator (nodes.get ()) != node_iterator ());

  std::copy (node_iterator (nodes.get ()), node_iterator (), std::ostream_iterator<node_base> (std::cout, " "));
  std::cout << std::endl;

  std::for_each (node_iterator (nodes.get ()), node_iterator (), boost::mem_fn (&node_base::double_me));

  std::copy (node_const_iterator (nodes.get ()), node_const_iterator (), std::ostream_iterator<node_base> (std::cout, "/"));
  std::cout << std::endl;
  return 0;
}