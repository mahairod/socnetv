#ifndef QCQUEUE_HPP
#define QCQUEUE_HPP

#include <QObject>
#include <QtCore/qqueue.h>
#include <QtCore/qlist.h>
#include <QReadWriteLock>

template <typename T>
class QCQueue : private QQueue<T> {
	mutable QReadWriteLock lock;
	typedef QQueue<T> super;
#define RLOCK QReadLocker rlocker(&lock)
#define WLOCK QWriteLocker wlocker(&lock)
public:
	QCQueue() {}

    QCQueue(const QCQueue<T> &l);
    ~QCQueue();
    QCQueue<T> &operator=(const QCQueue<T> &l);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QCQueue(QCQueue<T> &&other) Q_DECL_NOTHROW
        : super(other) {}
    inline QCQueue &operator=(QCQueue<T> &&other) Q_DECL_NOTHROW { QCQueue moved(std::move(other)); swap(moved); return *this; }
#endif
    inline void swap(QCQueue<T> &other) Q_DECL_NOTHROW {
		WLOCK;
		super::swap(other);
    }

#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QCQueue(std::initializer_list<T> args)
        : super(args) {}
#endif

    bool operator==(const QCQueue<T> &l) const {
		RLOCK;
		return super::operator==(l);
    }
    inline bool operator!=(const QCQueue<T> &l) const { return !(*this == l); }

    inline int size() const Q_DECL_NOTHROW {
		RLOCK;
		return super::size();
	}

    inline void detach() {
		WLOCK;
		super::detach();
    }

    inline void detachShared() {
		WLOCK;
		super::detachShared();
    }

    inline bool isDetached() const {
		RLOCK;
		return super::isDetached();
    }

#if !defined(QT_NO_UNSHARABLE_CONTAINERS)
    inline void setSharable(bool sharable) {
		WLOCK;
		super::setSharable(sharable);
    }
#endif

    inline bool isSharedWith(const QCQueue<T> &other) const Q_DECL_NOTHROW { return super::isSharedWith(other); }

    inline bool isEmpty() const Q_DECL_NOTHROW {
		RLOCK;
		super::isEmpty();
    }

    void clear() {
		WLOCK;
		super::clear();
    }

    const T &at(int i) const{
		RLOCK;
		return super::at(i);
    }

    const T &operator[](int i) const{
		RLOCK;
		return super::operator[](i);
    }

    T &operator[](int i){
		RLOCK;
		return super::operator[](i);
    }

    void reserve(int size){
		WLOCK;
		return super::reserve(size);
    }

    void append(const T &t){
		WLOCK;
		return super::append(t);
    }

    void append(const QList<T> &t) {
		WLOCK;
		return super::append(t);
    }

    void prepend(const T &t) {
		WLOCK;
		return super::prepend(t);
    }

    void insert(int i, const T &t) {
		WLOCK;
		return super::insert(i, t);
    }

    void replace(int i, const T &t) {
		WLOCK;
		return super::replace(i, t);
    }

    void removeAt(int i) {
		WLOCK;
		return super::removeAt(i);
    }

    int removeAll(const T &t) {
		WLOCK;
		return super::removeAll(t);
    }

    bool removeOne(const T &t) {
		WLOCK;
		return super::removeOne(t);
    }

    T takeAt(int i){
		RLOCK;
		return super::takeAt(i);
    }

    T takeFirst() {
		RLOCK;
		return super::takeFirst();
    }

    T takeLast() {
		RLOCK;
		return super::takeLast();
    }

    void move(int from, int to) {
		WLOCK;
		return super::move(from, to);
    }

    void swap(int i, int j){
		WLOCK;
		return super::replace(i, j);
    }

    int indexOf(const T &t, int from = 0) const {
		RLOCK;
		return super::indexOf(t, from);
    }

    int lastIndexOf(const T &t, int from = -1) const {
		RLOCK;
		return super::lastIndexOf(t, from);
    }

    bool contains(const T &t) const {
		RLOCK;
		return super::contains(t);
    }

    typedef typename super::iterator Iterator;
    typedef typename super::const_iterator ConstIterator;

    // stl style
    typedef std::reverse_iterator<typename super::iterator> reverse_iterator;
    typedef std::reverse_iterator<typename super::const_iterator> const_reverse_iterator;
    inline Iterator begin() { detach(); return super::begin(); }
    inline ConstIterator begin() const Q_DECL_NOTHROW { return super::begin(); }
    inline ConstIterator cbegin() const Q_DECL_NOTHROW { return super::begin(); }
    inline ConstIterator constBegin() const Q_DECL_NOTHROW { return super::begin(); }
    inline Iterator end() { detach(); return super::end(); }
    inline ConstIterator end() const Q_DECL_NOTHROW { return super::end(); }
    inline ConstIterator cend() const Q_DECL_NOTHROW { return super::end(); }
    inline ConstIterator constEnd() const Q_DECL_NOTHROW { return super::end(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const Q_DECL_NOTHROW { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const Q_DECL_NOTHROW { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const Q_DECL_NOTHROW { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const Q_DECL_NOTHROW { return const_reverse_iterator(begin()); }
    Iterator insert(Iterator before, const T &t);
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    // more Qt
    inline int count() const { return size(); }
    inline int length() const { return size(); } // Same as count()
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T& constFirst() const { return first(); }
    inline const T& first() const { Q_ASSERT(!isEmpty()); return at(0); }
    T& last() { Q_ASSERT(!isEmpty()); return *(--end()); }
    const T& last() const { Q_ASSERT(!isEmpty()); return at(count() - 1); }
    inline const T& constLast() const { return last(); }
    inline void removeFirst() { Q_ASSERT(!isEmpty()); erase(begin()); }
    inline void removeLast() { Q_ASSERT(!isEmpty()); erase(--end()); }
    inline bool startsWith(const T &t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(const T &t) const { return !isEmpty() && last() == t; }
    QList<T> mid(int pos, int length = -1) const;

    T value(int i) const;
    T value(int i, const T &defaultValue) const;

    // stl compatibility
    inline void push_back(const T &t) { append(t); }
    inline void push_front(const T &t) { prepend(t); }
    inline T& front() { return first(); }
    inline const T& front() const { return first(); }
    inline T& back() { return last(); }
    inline const T& back() const { return last(); }
    inline void pop_front() { removeFirst(); }
    inline void pop_back() { removeLast(); }
    inline bool empty() const { return isEmpty(); }


    inline void enqueue(const T &t) { append(t); }
    inline T dequeue() { return takeFirst(); }
    inline T &head() { return first(); }
    inline const T &head() const { return first(); }

};


#endif // QCQUEUE_HPP
