/* Разместите здесь код класса Optional */
#include <stdexcept>
#include <utility>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
public:
    Optional() = default;
    constexpr Optional(Optional const& other) {
        if (other.is_initialized_) {
            Reset();
            new(data_)T(*(reinterpret_cast<const T*>(other.data_)));
            is_initialized_ = true;
        }
    }
    
   explicit Optional(const T& value) {
        new(data_)T(value);
         is_initialized_ = true;
    }
    
    explicit Optional(T&& value) {
        new(data_)T(std::move(value));
         is_initialized_ = true;
    }
    
    Optional(Optional&& other) noexcept{
        if (other.is_initialized_) {
            Reset();
      
        new(data_)T(std::move(*(reinterpret_cast<T*>(other.data_))));
              is_initialized_ = true;
    }
        else Optional();
    }

  constexpr  Optional& operator=(const T& value) {
        if (!is_initialized_) {
           new(data_)T(value);
           is_initialized_ = true;
        }
        else {
            Value()=value;
  }
        return *this; 
  }
    
  constexpr  Optional& operator=(T&& rhs) {
       // if (HasValue()) Reset();
      if (!is_initialized_) {
           new(data_)T(std::move(rhs));
             is_initialized_ = true;
      }
          else
          {
              Value()=std::move(rhs);
          }
        return *this;
    }

    constexpr Optional& operator=(const Optional& rhs) {
        if (!HasValue()) { 
            if (rhs.is_initialized_) {//non empty to empty
                Reset();
                new(data_)T(*(reinterpret_cast<const T*>(rhs.data_)));
                is_initialized_ = true;
            }
            else {//empty to empty
                if (rhs.is_initialized_) {
                Reset();
              
            }}
        }
        else {
            if (!rhs.is_initialized_) { //empty to non empty
                Reset();
              
            }
            else { //non empty to non empty 
                Value() = rhs.Value();
                is_initialized_ = true;
            }
        }
        return *this;
    }
    Optional& operator=(Optional&& rhs) {
        if (!HasValue()) { 
            if (rhs.is_initialized_) {//non empty to empty
                Reset();
                  new(data_)T(std::move(*(reinterpret_cast<T*>(rhs.data_))));
                is_initialized_ = true;
            }
            else {//empty to empty
                Reset();
                is_initialized_ = false;
            }
        }
        else {
            if (!rhs.is_initialized_) { //empty to non empty
                Reset();
               
            }
            else { //non empty to non empty 
                 if (rhs.is_initialized_) {
                 Value() = std::move(rhs.Value());
                      is_initialized_ = true;
            }}
        }
        return *this;
    }
  
    ~Optional() {
    
        if (HasValue()) {
          reinterpret_cast<T*>(data_)->~T();
        }
           is_initialized_ = false;
    }

   [[nodiscard]] bool HasValue() const {
        if (is_initialized_) {
            return true;
        }
        else {
            return false;
        }
    }

    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста
    T& operator*() {
         return   *reinterpret_cast<T*>(data_);
        }
    
    const T& operator*() const {
          return  *reinterpret_cast<const T*>(data_);
        }
     
    T* operator->() {
            return reinterpret_cast<T*>(data_);
    }

    const T* operator->() const {
            return reinterpret_cast<const T*>(data_);
    }

    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
    T& Value() {
        if (is_initialized_) {
            return *(reinterpret_cast<T*>(data_));
        }
        else {
            throw  BadOptionalAccess();
        }
    }
    const T& Value() const {
        if (!is_initialized_) {
            throw  BadOptionalAccess();
        }
        else {
            
            return *(reinterpret_cast<const T*>(data_));
        }
    }

    //template <typename Func, typename... T>
    //void ApplyToMany(Func& l, T&&... vs ) {
    //  (...,l(std::forward<T>(vs)));
    //}
    
      template <typename... Args>
    void Emplace(Args&&... args) {
        Reset();
        new(data_)T(std::forward<Args>(args)...);
       
       is_initialized_ = true;
        
   
    }
    
    void Reset() {
        if (HasValue()) {
            reinterpret_cast<T*>(data_)->~T();
        }
        is_initialized_ = false;
    }

private:
    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;
};