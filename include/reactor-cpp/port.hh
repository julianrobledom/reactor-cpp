/*
 * Copyright (C) 2019 TU Dresden
 * All rights reserved.
 *
 * Authors:
 *   Christian Menard
 */

#ifndef REACTOR_CPP_PORT_HH
#define REACTOR_CPP_PORT_HH

#include <set>

#include "multiport_callback.hh"
#include "reactor.hh"
#include "value_ptr.hh"

namespace reactor {

enum class PortType { Input, Output };

class BasePort : public ReactorElement {
private:
  BasePort* inward_binding_{nullptr};
  std::set<BasePort*> outward_bindings_{};
  const PortType type_;
  
  multiport::LockedPortList active_ports_{};
  std::size_t index_ = 0;

  std::set<Reaction*> dependencies_{};
  std::set<Reaction*> triggers_{};
  std::set<Reaction*> anti_dependencies_{};

protected:
  bool present_{false};

  BasePort(const std::string& name, PortType type, Reactor* container)
      : ReactorElement(name, (type == PortType::Input) ? ReactorElement::Type::Input : ReactorElement::Type::Output,
                       container)
      , type_(type) {}

  BasePort(const std::string& name, PortType type, Reactor* container, multiport::LockedPortList active_ports, std::size_t index)
      : ReactorElement(name, (type == PortType::Input) ? ReactorElement::Type::Input : ReactorElement::Type::Output,
                       container)
      , type_(type), active_ports_(active_ports), index_(index) {}
  void base_bind_to(BasePort* port);
  void register_dependency(Reaction* reaction, bool is_trigger) noexcept;
  void register_antidependency(Reaction* reaction) noexcept;
  virtual void cleanup() = 0;

public:
  [[nodiscard]] inline auto is_input() const noexcept -> bool { return type_ == PortType::Input; }
  [[nodiscard]] inline auto is_output() const noexcept -> bool { return type_ == PortType::Output; }
  [[nodiscard]] inline auto is_present() const noexcept -> bool {
    if (has_inward_binding()) {
      return inward_binding()->is_present();
    }
    return present_;
  };

  [[nodiscard]] inline auto has_inward_binding() const noexcept -> bool { return inward_binding_ != nullptr; }
  [[nodiscard]] inline auto has_outward_bindings() const noexcept -> bool { return !outward_bindings_.empty(); }
  [[nodiscard]] inline auto has_dependencies() const noexcept -> bool { return !dependencies_.empty(); }
  [[nodiscard]] inline auto has_anti_dependencies() const noexcept -> bool { return !anti_dependencies_.empty(); }

  [[nodiscard]] inline auto inward_binding() const noexcept -> BasePort* { return inward_binding_; }
  [[nodiscard]] inline auto outward_bindings() const noexcept -> const auto& { return outward_bindings_; }

  [[nodiscard]] inline auto triggers() const noexcept -> const auto& { return triggers_; }
  [[nodiscard]] inline auto dependencies() const noexcept -> const auto& { return dependencies_; }
  [[nodiscard]] inline auto anti_dependencies() const noexcept -> const auto& { return anti_dependencies_; }
  
  inline auto activate() const -> bool {
    if (this->is_present()) {
        return false;
    }

    if (active_ports_.active_ports_ != nullptr && *active_ports_.strategy_ == multiport::Strategy::Callback) {
      auto calculated_index = (*active_ports_.size_)++;

      if (calculated_index >= active_ports_.active_ports_->capacity()) {
          throw std::runtime_error("setting to much ports");
      }

      if ( (calculated_index * 100) / active_ports_.active_ports_->capacity() > 20 ) {
        *active_ports_.strategy_ = multiport::Strategy::Linear;
      }

      (*active_ports_.active_ports_)[calculated_index] = index_;
      return true;
    }
    return false;
  }

  inline void clear() noexcept {
    present_ = false;

    if (active_ports_.active_ports_ != nullptr) {
      if (*active_ports_.size_ == 0) {
        return;
      } 

      active_ports_.size_->store(0);
      active_ports_.active_ports_->clear();
    }
  }

  inline void deactivate() noexcept {
    active_ports_.active_ports_ = nullptr;
    active_ports_.size_ = nullptr;
  }

  friend class Reaction;
  friend class Scheduler;
};

template <class T> class Port : public BasePort {
private:
  ImmutableValuePtr<T> value_ptr_{nullptr};

  void cleanup() noexcept final {
      value_ptr_ = nullptr; 
      clear();
  }

public:
  using value_type = T;

  Port(const std::string& name, PortType type, Reactor* container)
      : BasePort(name, type, container) {}
  Port(const std::string& name, PortType type, Reactor* container, multiport::LockedPortList active_ports, std::size_t index)
      : BasePort(name, type, container, active_ports, index) {}


  void bind_to(Port<T>* port) { base_bind_to(port); }
  [[nodiscard]] auto typed_inward_binding() const noexcept -> Port<T>*;
  [[nodiscard]] auto typed_outward_bindings() const noexcept -> const std::set<Port<T>*>&;

  void set(const ImmutableValuePtr<T>& value_ptr);
  void set(MutableValuePtr<T>&& value_ptr) { set(ImmutableValuePtr<T>(std::forward<MutableValuePtr<T>>(value_ptr))); }
  void set(const T& value) { set(make_immutable_value<T>(value)); }
  void set(T&& value) { set(make_immutable_value<T>(std::forward<T>(value))); }
  // Setting a port to nullptr is not permitted.
  void set(std::nullptr_t) = delete;
  void startup() final {}
  void shutdown() final {}

  auto get() const noexcept -> const ImmutableValuePtr<T>&;
};

template <> class Port<void> : public BasePort {
private:

  void cleanup() noexcept final { 
      clear();
  }

public:
  using value_type = void;

  Port(const std::string& name, PortType type, Reactor* container)
      : BasePort(name, type, container) {}

  Port(const std::string& name, PortType type, Reactor* container, multiport::LockedPortList active_ports, std::size_t index)
      : BasePort(name, type, container, active_ports, index) {}

  void bind_to(Port<void>* port) { base_bind_to(port); }
  [[nodiscard]] auto typed_inward_binding() const noexcept -> Port<void>*;
  [[nodiscard]] auto typed_outward_bindings() const noexcept -> const std::set<Port<void>*>&;

  void set();

  void startup() final {}
  void shutdown() final {}
};

template <class T> class Input : public Port<T> { // NOLINT
public:
  Input(const std::string& name, Reactor* container)
      : Port<T>(name, PortType::Input, container) {}
  Input(const std::string& name, Reactor* container, multiport::LockedPortList active_ports, std::size_t index)
      : Port<T>(name, PortType::Input, container, active_ports, index) {}

  Input(Input&&) = default; // NOLINT(performance-noexcept-move-constructor)
};

template <class T> class Output : public Port<T> { // NOLINT
public:
  Output(const std::string& name, Reactor* container)
      : Port<T>(name, PortType::Output, container) {}
  Output(const std::string& name, Reactor* container, multiport::LockedPortList active_ports, std::size_t index)
      : Port<T>(name, PortType::Output, container, active_ports, index) {}

  Output(Output&&) = default; // NOLINT(performance-noexcept-move-constructor)
};

} // namespace reactor

#include "impl/port_impl.hh"

#endif // REACTOR_CPP_PORT_HH
