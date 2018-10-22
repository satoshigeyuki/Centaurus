#include "CompositeATN.hpp"

namespace Centaurus
{
template<typename TCHAR>
void CompositeATN<TCHAR>::build_wildcard_closure(CATNClosure& closure, const Identifier& id, int color, ATNStateStack& stack) const
{
    //std::cout << "Wildcard " << id.narrow() << ":" << color << std::endl;

    for (const auto& p : m_dict)
    {
        for (int i = 0; i < p.second.get_num_nodes(); i++)
        {
            const CATNNode<TCHAR>& node = p.second[i];

            if (node.get_submachine() == id)
            {
                if (stack.find(p.first, i))
                {
                    //std::cout << "Upward sentinel reached." << std::endl;
                    continue;
                }
                stack.push(p.first, i);
                build_closure_exclusive(closure, ATNPath(p.first, i), color, stack);
                stack.pop();
            }
        }
    }
}
template<typename TCHAR>
void CompositeATN<TCHAR>::build_closure_exclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack) const
{
    const CATNNode<TCHAR>& node = get_node(path);

    //std::cout << "Exclusive " << path << ":" << color << std::endl;

    if (node.is_stop_node())
    {
        //std::cout << "Stop node " << path << std::endl;

        ATNPath parent = path.parent_path();

        if (parent.depth() == 0)
        {
            build_wildcard_closure(closure, path.leaf_id(), color, stack);
        }
        else
        {
            build_closure_exclusive(closure, parent, color, stack);
        }
    }
    else
    {
        //std::cout << "Intermediate node " << path << std::endl;

        for (const auto& tr : node.get_transitions())
        {
            if (tr.is_epsilon())
            {
                ATNPath dest_path = path.replace_index(tr.dest());

                const CATNNode<TCHAR>& dest_node = get_node(dest_path);

                if (dest_node.is_terminal())
                {
                    closure.emplace(dest_path, color);

                    build_closure_exclusive(closure, dest_path, color, stack);
                }
                else
                {
                    build_closure_inclusive(closure, dest_path.add(dest_node.get_submachine(), 0), color, stack);
                }
            }
        }
    }
}
template<typename TCHAR>
void CompositeATN<TCHAR>::build_closure_inclusive(CATNClosure& closure, const ATNPath& path, int color, ATNStateStack& stack) const
{
    const CATNNode<TCHAR>& node = get_node(path);

    if (node.is_terminal())
    {
        closure.emplace(path, color);

        build_closure_exclusive(closure, path, color, stack);
    }
    else
    {
        build_closure_inclusive(closure, path.add(node.get_submachine(), 0), color, stack);
    }
}
template<typename TCHAR>
void CompositeATN<TCHAR>::build_wildcard_departure_set(CATNDepartureSetFactory<TCHAR>& deptset_factory, const Identifier& id, int color, ATNStateStack& stack) const
{
    for (const auto& p : m_dict)
    {
        for (int i = 0; i < p.second.get_num_nodes(); i++)
        {
            if (p.second[i].get_submachine() == id)
            {
                if (stack.find(p.first, i))
                {
                    //std::cout << "Upward sentinel reached." << std::endl;
                    continue;
                }
                stack.push(p.first, i);
                build_departure_set_r(deptset_factory, ATNPath(p.first, i), color, stack);
                stack.pop();
            }
        }
    }
}
template<typename TCHAR>
void CompositeATN<TCHAR>::build_departure_set_r(CATNDepartureSetFactory<TCHAR>& deptset_factory, const ATNPath& path, int color, ATNStateStack& stack) const
{
    const CATNNode<TCHAR>& node = get_node(path);

    if (node.is_stop_node())
    {
        ATNPath parent_path = path.parent_path();

        if (parent_path.depth() == 0)
        {
            build_wildcard_departure_set(deptset_factory, path.leaf_id(), color, stack);
        }
        else
        {
            build_departure_set_r(deptset_factory, parent_path, color, stack);
        }
    }
    else
    {
        for (const auto& tr : node.get_transitions())
        {
            if (!tr.is_epsilon())
            {
                deptset_factory.add(tr.label(), path.replace_index(tr.dest()), color);
            }
        }
    }
}
template class CompositeATN<wchar_t>;
template class CompositeATN<unsigned char>;
template class CompositeATN<char>;
}
