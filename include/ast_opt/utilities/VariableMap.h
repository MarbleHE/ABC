#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VARIABLEMAP_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VARIABLEMAP_H_

#include <unordered_map>
#include <unordered_set>
#include "ast_opt/utilities/Scope.h"

template<typename T>
class VariableMap {
 private:
  std::unordered_map<ScopedIdentifier, T> map;

  /// Bool represents if it has been updated since last resetChangeFlags()
  std::unordered_set<ScopedIdentifier> changed;

 public:
  ~VariableMap() = default;

  VariableMap() = default;

  explicit VariableMap(const std::unordered_map<ScopedIdentifier, T> &&map) : map(std::move(map)) {};

  VariableMap(const VariableMap &other) = default;

  VariableMap(VariableMap &&other) noexcept = default;

  VariableMap &operator=(const VariableMap &other) = default;

  VariableMap &operator=(VariableMap &&other) noexcept = default;

  [[nodiscard]] const T &get(const ScopedIdentifier &s) const {
    return map.find(s)->second;
  }

  [[nodiscard]] const T &at(const ScopedIdentifier &s) const {
    return map.find(s)->second;
  }

  T erase(const ScopedIdentifier &s) {
    auto it = map.find(s);
    T v = it->second;
    map.erase(it);
    changed.erase(s);
    return v;
  }

  void add(const ScopedIdentifier &s, T v) {
    map.insert({s, v});
    changed.insert(s);
  }

  void insert_or_assign(ScopedIdentifier s, T &&v) {
    map.insert_or_assign(s, std::move(v));
    changed.insert(s);
  }

  void update(const ScopedIdentifier &s, T v) {
    auto it = map.find(s);
    if (it!=map.end()) {
      it->second = v;
      changed.insert(s);
    } else {
      throw std::invalid_argument("Cannot update value because no entry exists");
    }
  }

  bool has(const ScopedIdentifier &s) {
    return map.find(s)!=map.end();
  }

  void resetChangeFlags() {
    changed.clear();
  }

  auto begin() const { return map.begin(); }

  auto end() const { return map.end(); }

  [[nodiscard]] size_t count(ScopedIdentifier s) const { return map.count(s); }

  [[nodiscard]] const std::unordered_set<ScopedIdentifier> &changedEntries() const { return changed; }

};
#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VARIABLEMAP_H_
