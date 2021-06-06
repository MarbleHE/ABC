#include <utility>
#include <iostream>

#include "ast_opt/utilities/Scope.h"

const ScopedIdentifier &Scope::resolveIdentifier(const std::string &id) const {
  // go through scopes, starting from the current scope and then walking up (parent nodes), by looking for the given
  // identifier
  const Scope *curScope = this;
  while (curScope!=nullptr) {
    auto it = std::find_if(curScope->identifiers.begin(),
                           curScope->identifiers.end(),
                           [&id](const auto &p) { return p->getId()==id; });
    if (it!=curScope->identifiers.end()) {
      return **it;
    }
    curScope = curScope->parent;
  }

  throw std::runtime_error("Identifier (" + id + ") cannot be resolved!");
}

ScopedIdentifier &Scope::resolveIdentifier(const std::string &id) {
  // removes const from result of const counterpart, see https://stackoverflow.com/a/856839/3017719
  return const_cast<ScopedIdentifier &>(const_cast<const Scope *>(this)->resolveIdentifier(id));
}

void Scope::addIdentifier(const std::string &id) {

  if (!identifierIsLocal(id)) {
    // Warning if local variable shadows an outer one
    if (identifierExists(id)) {
      auto si = resolveIdentifier(id);
      std::cout << "WARNING: Variable with name " << si.getId() << " already exists in scope "
                << si.getScope().getScopeName() << " and the one in scope "
                << getScopeName() << " will shadow this one." << std::endl;
    }

    //std::cout << "Adding " << id << " to scope " << this->getScopeName() << "( scope: " << this << ", node: " << astNode
    //          << ")" << std::endl;
    identifiers.emplace(std::make_unique<ScopedIdentifier>(*this, id));
  } //else {
  //std::cout << "Adding " << id << " is ignored since already exits in scope " << this->getScopeName() << std::endl;
  //}
}

void Scope::addIdentifier(std::unique_ptr<ScopedIdentifier> &&scopedIdentifier) {
  if (&scopedIdentifier->getScope()!=this) {
    throw std::runtime_error(
        "Cannot add scoped identifier to a scope that differs from the scope specified in the scoped identifier.");
  }

  if (!identifierIsLocal(scopedIdentifier->getId())) {
    // Warning if local variable shadows an outer one
    if (identifierExists(scopedIdentifier->getId())) {
      auto si = resolveIdentifier(scopedIdentifier->getId());
      std::cout << "WARNING: Variable with name " << si.getId() << " already exists in scope "
                << si.getScope().getScopeName() << " and the one in scope "
                << getScopeName() << " will shadow this one." << std::endl;
    }

    //std::cout << "Adding " << scopedIdentifier->getId() << " to scope " << this->getScopeName() << std::endl;
    identifiers.insert(std::move(scopedIdentifier));
  } //  else {
  //  std::cout << "Adding " << scopedIdentifier->getId() << " is ignored since already exits in scope "
  //            << this->getScopeName() << std::endl;
  //  }

}

void Scope::addIdentifiers(std::initializer_list<std::string> ids) {
  std::for_each(ids.begin(), ids.end(), [&](const std::string &id) {
    addIdentifier(id);
  });
}

Scope &Scope::getParentScope() {
  return *parent;
}

const Scope &Scope::getParentScope() const {
  return *parent;
}

AbstractNode *Scope::getNodePtr() const {
  return astNode;
}

Scope *Scope::createNestedScope(Scope &parentScope, AbstractNode &scopeOpener) {

  // if a scope already exists, return it
  for (auto &s: parentScope.nestedScopes) {
    if (s->astNode==&scopeOpener) {
      //std::cout << "Not creating a scope for " << scopeOpener.getUniqueNodeId() << "(" << &scopeOpener << ") in scope "
      //          << parentScope.getScopeName() << " (" << &parentScope << ") since one already exists." << std::endl;
      return s.get();
    }
  }

  // Alternatively, do create a new scope
  auto scope = std::make_unique<Scope>(scopeOpener);
  Scope *scopePtr = scope.get();
  scope->setParent(&parentScope);
  scope->astNode = &scopeOpener;
  //std::cout << "Creating a new scope " << scope->getScopeName() << " ( " << &scope << ") in "
  //          << parentScope.getScopeName() << " (" << &parentScope << ")" << std::endl;
  parentScope.nestedScopes.push_back(std::move(scope));
  return scopePtr;
}

void Scope::setParent(Scope *parentScope) {
  Scope::parent = parentScope;
}

Scope::Scope(AbstractNode &abstractNode) : astNode(&abstractNode) {}

//Scope::Scope(const Scope &other) : astNode(other.astNode), parent(other.parent) {
//
//  // Create copies of identifiers
//  for (auto &si : other.identifiers) {
//    identifiers.insert(std::make_unique<ScopedIdentifier>(*this, si->getId()));
//  }
//
//  // Recursively copy nested scopes
//  nestedScopes.reserve(other.nestedScopes.size());
//  for (auto &s : other.nestedScopes) {
//    nestedScopes.emplace_back(std::make_unique<Scope>(*s));
//  }
//
//}

bool Scope::identifierExists(const std::string &id) const {
  const Scope *curScope = this;
  while (curScope!=nullptr) {
    auto it = std::find_if(curScope->identifiers.begin(),
                           curScope->identifiers.end(),
                           [&id](const auto &p) { return p->getId()==id; });
    if (it!=curScope->identifiers.end()) {
      return true;
    }
    curScope = curScope->parent;
  }
  return false;
}

bool Scope::identifierIsLocal(const std::string &id) const {
  // go through all identifiers that are declared in this scope and check whether the identifier matches the given one
  return std::any_of(identifiers.begin(), identifiers.end(), [id](const auto &scopedIdentifier) {
    return scopedIdentifier->getId()==id;
  });
}
std::string Scope::getScopeName() const {
  if (astNode) return astNode->getUniqueNodeId();
  else return "ScopeForNullptr";
}

Scope &Scope::getNestedScopeByCreator(AbstractNode &node) {
  for (auto &scope : nestedScopes) {
    if (*scope->astNode==node) return *scope;
  }
  throw std::runtime_error("Requested nested scope (created by " + node.getUniqueNodeId() + ") not found!");
}

const Scope &Scope::getNestedScopeByCreator(AbstractNode &node) const {
  for (auto &scope : nestedScopes) {
    if (*scope->astNode==node) return *scope;
  }
  throw std::runtime_error("Requested nested scope (created by " + node.getUniqueNodeId() + ") not found!");
}

Scope &ScopedIdentifier::getScope() {
  return *scope;
}

const Scope &ScopedIdentifier::getScope() const {
  return *scope;
}

const std::string &ScopedIdentifier::getId() const {
  return id;
}

std::string &ScopedIdentifier::getId() {
  return id;
}

ScopedIdentifier::ScopedIdentifier(Scope &scope, std::string id) : scope(&scope), id(std::move(id)) {}
