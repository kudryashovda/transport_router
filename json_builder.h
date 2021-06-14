#pragma once

#include "json.h"

#include <functional>
#include <memory>
#include <stack>

namespace json {
    /*
    The main idea is to save directives and values in std::vector, marking
    directives by IS_MARKER and Values by !IS_MARKER
    until EndDict() or EndArray() is reached.
    If they are reached search previously saved marked directive in the array
    and create Node from intermeadiate values and save them to the
    previously marked directive place.
    */

    inline bool IS_MARKER = true;

    using u_ptr_Node = std::unique_ptr<json::Node>;

    struct SNode {
        u_ptr_Node u_ptr;
        bool is_marker;
    };

    class Builder;
    class StartX;
    class KeyContext;
    class StartArrayContext;
    class DictItemContext;

    class DictItemContext {
    public:
        DictItemContext(Builder& builder)
            : builder_(builder){};

        KeyContext& Key(const std::string& key);
        Builder& EndDict();

    private:
        Builder& builder_;
    };

    class StartX {
    public:
        StartX(Builder& builder)
            : builder_(builder){};

        DictItemContext& StartDict();
        StartArrayContext& StartArray();

    private:
        Builder& builder_;
    };

    class KeyContext : public StartX {
    public:
        KeyContext(Builder& builder)
            : StartX(builder)
            , builder_(builder){};

        DictItemContext& Value(const Node::Value& value);

    private:
        Builder& builder_;
    };

    class StartArrayContext : public StartX {
    public:
        StartArrayContext(Builder& builder)
            : StartX(builder)
            , builder_(builder){};

        StartArrayContext& Value(const Node::Value& value);
        Builder& EndArray();

    private:
        Builder& builder_;
    };

    class Builder : public DictItemContext, KeyContext, StartArrayContext {
    public:
        friend StartArrayContext;

        Builder()
            : DictItemContext(*this)
            , KeyContext(*this)
            , StartArrayContext(*this) {
        }

        DictItemContext& StartDict();
        StartArrayContext& StartArray();

        Builder& EndDict();
        Builder& EndArray();

        KeyContext& Key(const std::string& key);
        Builder& Value(const Node::Value& value);

        Node& Build();

    private:
        void CreateNode(const Node::Value& value, bool is_marker);
        void CheckIfOblectCompleated();

    private:
        Node root_ = nullptr;
        std::vector<SNode> m_nodes_stack_;
    };

} // namespace json