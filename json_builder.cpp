#include "json_builder.h"

namespace json {
    using namespace std::string_literals;

    // --- DictItemContext ---
    KeyContext& DictItemContext::Key(const std::string& key) {
        return builder_.Key(key);
    }

    Builder& DictItemContext::EndDict() {
        return builder_.EndDict();
    }

    // --- StartXContext ---
    DictItemContext& StartX::StartDict() {
        return builder_.StartDict();
    }

    StartArrayContext& StartX::StartArray() {
        return builder_.StartArray();
    }

    // --- StartArrayContext ---
    StartArrayContext& StartArrayContext::Value(const Node::Value& value) {
        builder_.CreateNode(value, !IS_MARKER);

        return *this;
    }

    Builder& StartArrayContext::EndArray() {
        return builder_.EndArray();
    }

    // --- KeyContext ---
    DictItemContext& KeyContext::Value(const Node::Value& value) {
        return builder_.Value(value);
    }

    // --- Builder ---
    void Builder::CreateNode(const Node::Value& value, bool is_marker) {
        auto lambdaNodeCreator = [this, is_marker](auto&& value) {
            m_nodes_stack_.emplace_back(SNode{ std::make_unique<Node>(value), is_marker });
        };
        std::visit(lambdaNodeCreator, value);
    }

    DictItemContext& Builder::StartDict() {
        Builder::CreateNode(Dict(), IS_MARKER);

        return *this;
    }

    Builder& Builder::EndDict() {
        Builder::CheckIfOblectCompleated();

        std::stack<std::pair<std::string, u_ptr_Node>> tmp_key_value_stack;

        while (!(m_nodes_stack_.back().is_marker && m_nodes_stack_.back().u_ptr->IsDict())) {
            u_ptr_Node value = std::move(m_nodes_stack_.back().u_ptr);
            m_nodes_stack_.pop_back();

            if (m_nodes_stack_.empty()) {
                throw std::logic_error("No StartDict() or StartArray() was found..."s);
            }

            if (!m_nodes_stack_.back().u_ptr->IsString()) {
                throw std::logic_error("No StartDict() or StartArray() was found..."s);
            }

            std::string key = m_nodes_stack_.back().u_ptr->AsString();
            m_nodes_stack_.pop_back();

            tmp_key_value_stack.emplace(std::move(key), std::move(value));

            if (m_nodes_stack_.empty()) {
                throw std::logic_error("No StartDict() or StartArray() was found..."s);
            }
        }

        m_nodes_stack_.pop_back(); // delete Dict() marker

        Dict tmp_dict; // generate new temp Dict
        while (!tmp_key_value_stack.empty()) {
            tmp_dict[tmp_key_value_stack.top().first] = std::move(*(tmp_key_value_stack.top().second));
            tmp_key_value_stack.pop();
        }

        Builder::CreateNode(tmp_dict, !IS_MARKER);

        return *this;
    }

    KeyContext& Builder::Key(const std::string& key) {
        if (m_nodes_stack_.back().is_marker && !m_nodes_stack_.back().u_ptr->IsDict()) {
            throw std::logic_error("Called Key() not after Dict()..."s);
        }

        Builder::CreateNode(key, IS_MARKER); //  do not put here '!'

        return *this;
    }

    Builder& Builder::Value(const Node::Value& value) {
        Builder::CheckIfOblectCompleated();

        Builder::CreateNode(value, !IS_MARKER);

        return *this;
    }

    StartArrayContext& Builder::StartArray() {
        Builder::CreateNode(Array(), IS_MARKER);

        return *this;
    }

    Builder& Builder::EndArray() {
        Builder::CheckIfOblectCompleated();

        std::stack<u_ptr_Node> tmp_nodes_stack;

        while (!(m_nodes_stack_.back().is_marker && m_nodes_stack_.back().u_ptr->IsArray())) {
            tmp_nodes_stack.push(std::move(m_nodes_stack_.back().u_ptr));
            m_nodes_stack_.pop_back();

            if (m_nodes_stack_.empty()) {
                throw std::logic_error("No StartDict() or StartArray() was found..."s);
            }
        }

        m_nodes_stack_.pop_back(); // delete Array() marker

        Array tmp_array; // generate new array
        while (!tmp_nodes_stack.empty()) {
            tmp_array.push_back(*tmp_nodes_stack.top());
            tmp_nodes_stack.pop();
        }

        Builder::CreateNode(tmp_array, !IS_MARKER);

        return *this;
    }

    Node& Builder::Build() {
        if (m_nodes_stack_.empty()) {
            throw std::logic_error("Called Build() after constructor..."s);
        }

        if (m_nodes_stack_.back().is_marker || m_nodes_stack_.front().is_marker) {
            throw std::logic_error("Unconstructed node..."s);
        }

        root_ = Node(*m_nodes_stack_.front().u_ptr);

        return root_;
    }

    void Builder::CheckIfOblectCompleated() {
        constexpr int COMPLEATED_OBJECT_ELEMENTS_COUNT = 1;
        bool obj_is_completed = (m_nodes_stack_.size() == COMPLEATED_OBJECT_ELEMENTS_COUNT
                                 && !m_nodes_stack_.back().is_marker);

        if (obj_is_completed) {
            throw std::logic_error("Node is already compleated..."s);
        }
    }

} // namespace json