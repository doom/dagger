/*
** Created by doom on 28/06/19.
*/

#ifndef DAGGER_DAGGER_HPP
#define DAGGER_DAGGER_HPP

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <string_view>

/**
 * Implementation of a DAWG (Directed Acyclic Word Graph), which is a graph describing a set of words.
 *
 * Unlike a trie (in which paths that have diverged cannot converge again), the DAWG factorizes common paths,
 * even those at the end of words, thus minimizing its size.
 *
 * In order to build the DAWG from a sorted dictionary (using ASCII ascending order), the algorithm described by
 * Daciuk et al. in the following paper:
 * "Incremental Construction of Minimal Acyclic Finite-State Automata" (https://www.aclweb.org/anthology/J00-1002.pdf)
 */

namespace doom
{
    namespace details
    {
        class vertex
        {
        private:
            std::map<char, vertex *> edges_;
            bool is_accepting_{false};

        public:
            const auto &edges() const noexcept
            {
                return edges_;
            }

            auto &edges() noexcept
            {
                return edges_;
            }

            bool is_word_end() const noexcept
            {
                return is_accepting_;
            }

            void mark_word_end() noexcept
            {
                is_accepting_ = true;
            }

            friend bool operator<(const vertex &lhs, const vertex &rhs) noexcept
            {
                if (&lhs == &rhs) {
                    return false;
                }

                if (lhs.is_word_end() != rhs.is_word_end()) {
                    return lhs.is_word_end() == false;
                }

                return lhs.edges() < rhs.edges();
            }
        };

        struct unminimized_vertex
        {
            std::unique_ptr<vertex> vertex_;
            char edge_from_parent_;
        };

        struct dereferencing_less
        {
            using is_transparent = std::true_type;

            template <typename A, typename B>
            constexpr bool operator()(A &&a, B &&b) const noexcept
            {
                return *a < *b;
            }
        };
    }

    class dagger
    {
    private:
        using vertex = details::vertex;
        using unminimized_vertex = details::unminimized_vertex;
        using dereferencing_less = details::dereferencing_less;

        std::string prev_word_;

        std::unique_ptr<vertex> root_vertex_{std::make_unique<vertex>()};

        /** The stack describing the current unminimized path */
        std::stack<unminimized_vertex> unminimized_path_;

        /** The set containing all the minimized vertices */
        std::set<std::unique_ptr<vertex>, dereferencing_less> minimized_vertices_;

    private:
        void finish_minimization() noexcept
        {
            /** Minimize the remaining path */
            minimize_until(0);
        }

        void minimize_until(size_t until) noexcept
        {
            while (unminimized_path_.size() > until) {
                auto top = std::move(unminimized_path_.top());
                unminimized_path_.pop();

                /** Look for an identical, already minimized vertex */
                if (auto it = minimized_vertices_.find(top.vertex_); it != minimized_vertices_.end()) {
                    /** An identical vertex was found, update the parent to point to it, and drop the popped vertex */
                    if (not unminimized_path_.empty()) {
                        /** Attach the vertex to the parent, which is the next unminimized node */
                        unminimized_path_.top().vertex_->edges()[top.edge_from_parent_] = it->get();
                    } else {
                        /** We reached the root of the path to minimize, attach to the root of the graph */
                        root_vertex_->edges()[top.edge_from_parent_] = it->get();
                    }
                } else {
                    /** No such vertex was found, insert it */
                    minimized_vertices_.emplace(std::move(top.vertex_));
                }
            }
        }

        void add_suffix(std::string_view suffix) noexcept
        {
            auto parent_vertex = unminimized_path_.empty() ? root_vertex_.get()
                                                           : unminimized_path_.top().vertex_.get();

            /** Create a path with the all the characters from the suffix, and add its vertices
             * in the stack of vertices to process */
            for (auto &&c : suffix) {
                auto new_vertex = std::make_unique<vertex>();
                parent_vertex->edges()[c] = new_vertex.get();
                unminimized_path_.push({std::move(new_vertex), c});
                parent_vertex = unminimized_path_.top().vertex_.get();
            }

            /** The last vertex is the end of the word, mark it as such */
            parent_vertex->mark_word_end();
        }

        void insert(std::string_view word) noexcept
        {
            /** Determine at which point the previous word and the current word differ. The part before that point
             * can then safely be ignored, because it already has been processed when the previous word was inserted */
            auto[it1, _] = std::mismatch(prev_word_.begin(), prev_word_.end(), word.begin(), word.end());
            auto common_prefix_len = it1 - prev_word_.begin();

            /** Minimize from the end of the previous word, leaving only the common prefix */
            minimize_until(common_prefix_len);

            /** Create the path for the rest of the word */
            add_suffix(word.substr(common_prefix_len));

            /** Save the word until the next insertion */
            prev_word_ = word;
        }

        dagger() = default;

    public:
        dagger(const dagger &) = delete;

        dagger &operator=(const dagger &) = delete;

        dagger(dagger &&other) = default;

        dagger &operator=(dagger &&other) = default;

        bool contains(std::string_view word) noexcept
        {
            auto current_vertex = root_vertex_.get();

            for (auto &&c : word) {
                auto it = current_vertex->edges().find(c);

                if (it == current_vertex->edges().end()) {
                    return false;
                }
                current_vertex = it->second;
            }
            return current_vertex->is_word_end();
        }

        template <typename ForwardIterator>
        static dagger from_dictionary(ForwardIterator begin, ForwardIterator end)
        {
            dagger dag;

            for (; begin != end; ++begin) {
                dag.insert(*begin);
            }
            dag.finish_minimization();
            return dag;
        }
    };
}

#endif /* !DAGGER_DAGGER_HPP */
