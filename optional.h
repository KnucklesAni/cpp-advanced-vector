#include <stdexcept>
#include <utility>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
  using exception::exception;

  virtual const char *what() const noexcept override {
    return "Bad optional access";
  }
};

template <typename T> class Optional {
public:
  Optional() = default;
  Optional(const T &value);
  Optional(T &&value);
  Optional(const Optional &other);
  Optional(Optional &&other);

  Optional &operator=(const T &value);
  Optional &operator=(T &&rhs);
  Optional &operator=(const Optional &rhs);
  Optional &operator=(Optional &&rhs);

  ~Optional();

  bool HasValue() const;

  // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
  // Эти проверки остаются на совести программиста
  T &operator*() &;
  const T &operator*() const &;
  T *operator->();
  const T *operator->() const;
  T &&operator*() &&;

  // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
  T &Value() &;
  const T &Value() const &;
  T &&Value() &&;

  void Reset();

  template <typename... Vs> void Emplace(Vs &&...vs);

private:
  // alignas нужен для правильного выравнивания блока памяти
  alignas(T) char data_[sizeof(T)];
  bool is_initialized_ = false;
};

template <typename T>
Optional<T>::Optional(const T &value) : is_initialized_(true) {
  new (&data_[0]) T(value);
}

template <typename T> Optional<T>::Optional(T &&value) : is_initialized_(true) {
  new (&data_[0]) T(std::move(value));
}

template <typename T>
Optional<T>::Optional(const Optional &other)
    : is_initialized_(other.is_initialized_) {
  if (is_initialized_) {
    new (&data_[0]) T(other.Value());
  }
}

template <typename T>
Optional<T>::Optional(Optional &&other)
    : is_initialized_(std::move(other.is_initialized_)) {
  if (is_initialized_) {
    new (&data_[0]) T(std::move(other.Value()));
  }
}

template <typename T> Optional<T>::~Optional() { Reset(); }

template <typename T> Optional<T> &Optional<T>::operator=(const T &value) {
  if (!is_initialized_) {
    new (&data_[0]) T(value);
    is_initialized_ = true;
  } else {
    *(reinterpret_cast<T *>(&data_[0])) = value;
  }
  return *this;
}

template <typename T> Optional<T> &Optional<T>::operator=(T &&rhs) {
  if (!is_initialized_) {
    new (&data_[0]) T(std::move(rhs));
    is_initialized_ = true;
  } else {
    *reinterpret_cast<T *>(&data_[0]) = std::move(rhs);
  }
  return *this;
}

template <typename T> Optional<T> &Optional<T>::operator=(const Optional &rhs) {
  if (!is_initialized_) {
    if (rhs.is_initialized_) {
      new (&data_[0]) T(rhs.Value());
      is_initialized_ = rhs.is_initialized_;
    }
  } else {
    if (rhs.is_initialized_) {
      *reinterpret_cast<T *>(&data_[0]) = rhs.Value();
    } else {
      Reset();
    }
  }
  return *this;
}

template <typename T> Optional<T> &Optional<T>::operator=(Optional &&rhs) {
  if (!is_initialized_) {
    if (rhs.is_initialized_) {
      new (&data_[0]) T(std::move(rhs.Value()));
      is_initialized_ = std::move(rhs.is_initialized_);
    }
  } else {
    if (rhs.is_initialized_) {
      *reinterpret_cast<T *>(&data_[0]) = std::move(rhs.Value());
    } else {
      Reset();
    }
  }
  return *this;
}

template <typename T> bool Optional<T>::HasValue() const {
  return is_initialized_;
}

template <typename T> T &Optional<T>::operator*() & {
  return *reinterpret_cast<T *>(&data_[0]);
}

template <typename T> const T &Optional<T>::operator*() const & {
  return *reinterpret_cast<const T *>(&data_[0]);
}

template <typename T> T &&Optional<T>::operator*() && {
  return std::move(*reinterpret_cast<T *>(&data_[0]));
}

template <typename T> T *Optional<T>::operator->() {
  return reinterpret_cast<T *>(&data_[0]);
}

template <typename T> const T *Optional<T>::operator->() const {
  return reinterpret_cast<const T *>(&data_[0]);
}

template <typename T> T &Optional<T>::Value() & {
  if (!is_initialized_) {
    throw BadOptionalAccess();
  }
  return *reinterpret_cast<T *>(&data_[0]);
}

template <typename T> const T &Optional<T>::Value() const & {
  if (!is_initialized_) {
    throw BadOptionalAccess();
  }
  return *reinterpret_cast<const T *>(&data_[0]);
}

template <typename T> T &&Optional<T>::Value() && {
  if (!is_initialized_) {
    throw BadOptionalAccess();
  }
  return std::move(*reinterpret_cast<T *>(&data_[0]));
}

template <typename T> void Optional<T>::Reset() {
  if (is_initialized_) {
    reinterpret_cast<T *>(&data_[0])->~T();
  }
  is_initialized_ = false;
}

template <typename T>
template <typename... Vs>
void Optional<T>::Emplace(Vs &&...vs) {
  if (HasValue()) {
    Reset();
  }
  new (&data_[0]) T(std::forward<Vs>(vs)...);
  is_initialized_ = true;
}