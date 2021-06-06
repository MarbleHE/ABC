#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_SCOPE_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_SCOPE_H_

#include <tuple>
#include <unordered_set>
#include "ast_opt/ast/AbstractNode.h"

// forward declarations
class Scope;

class ScopedIdentifier {
 private:
  /// (weak) pointer to the Scope this identifier belongs to
  Scope *scope;

  /// identifier (e.g., variable's name)
  std::string id;

 public:
  ~ScopedIdentifier() = default;

  ScopedIdentifier() = default;

  ScopedIdentifier(const ScopedIdentifier &other) = default;

  ScopedIdentifier(ScopedIdentifier &&other) = default;

  ScopedIdentifier &operator=(const ScopedIdentifier &other) = default;

  ScopedIdentifier &operator=(ScopedIdentifier &&other) = default;

  /// Creates a new ScopedIdentifier.
  /// \param scope The scope where the identifier was declared in.
  /// \param id The identifier that is defined in the given scope.
  ScopedIdentifier(Scope &scope, std::string id);

  /// Gets the scope associated with this ScopedIdentifier.
  /// \return (A reference to) the scope of this ScopedIdentifier.
  Scope &getScope();

  /// Gets the scope associated with this ScopedIdentifier.
  /// \return (A const reference to) the scope of this ScopedIdentifier.
  [[nodiscard]] const Scope &getScope() const;

  /// Gets the identifier associated with this ScopedIdentifier.
  /// \return (A const string reference to) the identifier of this ScopedIdentifier.
  [[nodiscard]] const std::string &getId() const;

  /// Gets the identifier associated with this ScopedIdentifier.
  /// \return (A string reference to) the identifier of this ScopedIdentifier.
  std::string &getId();
};

class Scope {
 private:
  /// (Weak) pointer to the AST node that creates this scope
  AbstractNode *astNode;

  /// Set of identifiers declared in this scope
  std::unordered_set<std::unique_ptr<ScopedIdentifier>> identifiers;

  /// Parent scope (if it exists)
  Scope *parent = nullptr;

  /// The scopes that are nested in this scope
  std::vector<std::unique_ptr<Scope>> nestedScopes;

 public:
  /// Destructor
  ~Scope() = default;

  /// Creates a new scope.
  /// \param abstractNode The node that creates this scope.
  explicit Scope(AbstractNode &abstractNode);

//  /// Copy Constructor
//  Scope(const Scope&);

  /// Move Constructor
  /// \param other the scope to move
  Scope(Scope &&other) = default;

  /// Copy Assignment
  Scope &operator=(const Scope &) = delete;

  /// Move Assignment
  Scope &operator=(Scope &&) noexcept = default;

  /// Adds an declared identifier (e.g., variable) to this scope.
  /// \param id The identifier to be added to this scope.
  void addIdentifier(const std::string &id);

  /// Adds the given ScopedIdentifier to this scope. Checks that the scopedIdentifier is actually from this scope.
  /// \param scopedIdentifier The scoped identifier to be added to this scope.
  void addIdentifier(std::unique_ptr<ScopedIdentifier> &&scopedIdentifier);

  /// Adds multiple declared identifier (e.g., variable) to this scope.
  /// \param ids (A list of) identifiers to be added to this scope.
  void addIdentifiers(std::initializer_list<std::string> ids);

  /// Checks whether the given identifier is local, i.e., declared in this scope and not in any parent scope.
  /// \param id The identifier to be checked.
  /// \return True iff the given identifier id is defined in this scope.
  [[nodiscard]] bool identifierIsLocal(const std::string &id) const;

  /// Checks whether the given identifier id exists in this or any parent scope.
  /// \param id The identifier to be checked.
  /// \return True iff the given identifier is defined in this or any parent scope.
  [[nodiscard]] bool identifierExists(const std::string &id) const;

  /// Sets the parent scope of this scope.
  /// \param parentScope The scope to be set as parent of this scope.
  void setParent(Scope *parentScope);

  /// Gets the parent scope of this scope.
  /// \return (A reference to) the parent scope of this scope.
  Scope &getParentScope();

  /// Gets the parent scope of this scope.
  /// \return (A const reference to) the parent scope of this scope.
  [[nodiscard]] const Scope &getParentScope() const;

  /// Returns the pointer to the AST node this is associated with
  /// WARNING: This can be nullptr or it can even be INVALID!
  /// \return pointer to the AST node this is associated with
  AbstractNode *getNodePtr() const;

  /// Creates a nested scope with the given parentScope as parent and the given AbstractNode scopeOpener as the AST node
  /// that creates this scope.
  /// \param parentScope The parent scope of the scope to be created.
  /// \param scopeOpener The AST node that creates the nested scope.
  /// \return (A weak pointer) to the nested scope that is created.
  static Scope *createNestedScope(Scope &parentScope, AbstractNode &scopeOpener);

  /// Determines the ScopedIdentifier of the given identifier.
  /// \param id The identifier for that the ScopedIdentifier should be determined.
  /// \return (A const reference) to the ScopedIdentifier object associated with the given identifier.
  [[nodiscard]] const ScopedIdentifier &resolveIdentifier(const std::string &id) const;

  /// Determines the ScopedIdentifier of the given identifier.
  /// \param id The identifier for that the ScopedIdentifier should be determined.
  /// \return (A const reference) to the ScopedIdentifier object associated with the given identifier.
  ScopedIdentifier &resolveIdentifier(const std::string &id);

  /// Gets the nested scoped that is created by the given node. Note that this does NOT include deeper
  /// nested scopes. It only considers scopes that are directly (next level) nested into this scope.
  /// \param node The node that created the scope that is searched for.
  /// \return (A reference to) the scope created by the given node.
  Scope &getNestedScopeByCreator(AbstractNode &node);

  /// Gets the nested scoped that is created by the given node. Note that this does NOT include deeper
  /// nested scopes. It only considers scopes that are directly (next level) nested into this scope.
  /// \param node The node that created the scope that is searched for.
  /// \param node The node that created the scope that is searched for.
  /// \return (A const reference to) the scope created by the given node.
  const Scope &getNestedScopeByCreator(AbstractNode &node) const;

  /// Get Scope name
  /// \return the name of this scope (uniqueID of the associated AST node)
  [[nodiscard]] std::string getScopeName() const;
};

namespace std {
template<>
struct hash<ScopedIdentifier> {
  size_t operator()(const ScopedIdentifier &s) const {
    return std::hash<std::string>{}(s.getScope().getScopeName() + "::" + s.getId());
  }
};

template<>
struct equal_to<ScopedIdentifier> {
  bool operator()(ScopedIdentifier const &s1, ScopedIdentifier const &s2) const {
    return s1.getId()==s2.getId() && s1.getScope().getScopeName()==s2.getScope().getScopeName();
  }
};
}
#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_SCOPE_H_
