#include <xcb/xcb.h>

namespace clipboardxx {
namespace xcb {

    using Atom = xcb_atom_t;
    using Window = xcb_window_t;

    class Event {
    public:
        // TODO: use constexpr case for enums
        enum Type {
            NONE = 0,
            REQUEST_SELECTION,
            SELECTION_CLEAR,
            SELECTION_NOTIFY
        };

        Event(Type type) : m_type(type) {}
        Type get_type() const { return m_type; }

    private:
        Type m_type;
    };

    class RequestSelectionEvent : public Event {
    public:
        RequestSelectionEvent(Window requestor, Window owner, Atom selection, Atom target, Atom property) :
            Event(Type::REQUEST_SELECTION), m_requestor(requestor), m_owner(owner),
                m_selection(selection), m_target(target), m_property(property) {}

        const Window m_requestor, m_owner;
        const Atom m_selection, m_target, m_property;
    };

    class SelectionNotifyEvent : public Event {
    public:
        SelectionNotifyEvent(Window requestor, Atom selection, Atom target, Atom property) : Event(Type::SELECTION_NOTIFY),
            m_requestor(requestor), m_selection(selection), m_target(target), m_property(property) {}

        const Window m_requestor;
        const Atom m_selection, m_target, m_property;
    };

    class SelectionClearEvent : public Event {
    public:
        SelectionClearEvent(Atom selection) : Event(Type::SELECTION_CLEAR), m_selection(selection) {}

        const Atom m_selection;
    };
}
}
