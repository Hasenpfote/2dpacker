/**
 * @file packer.h
 * @brief 二次元矩形パッキング
 */
#pragma once
#include <vector>

/**
 * @class Rect
 */
class Rect{
public:
	inline void set(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
		this->x = x; this->y = y; this->w = w; this->h = h;
	}
	inline unsigned int getX() const { return x; }
	inline unsigned int getY() const { return y; }
	inline unsigned int getW() const { return w; }
	inline unsigned int getH() const { return h; }
	inline bool contains(unsigned int w, unsigned int h) const {
		return ((w > this->w) || (h > this->h))? false : true;
	}
private:
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
};

/**
 * @class Node
 */
class Node{
public:
	Node();
	~Node();
public:
	Node* sibling;
	Node* right;
	Node* bottom;
	Rect rect;
	bool use;
};

/**
 * @class Data
 */
class Data{
public:
	Data(){ w = 0; h = 0; fit = NULL; }
	Data(unsigned int w, unsigned int h){ this->w = w; this->h = h; fit = NULL; }
	Data(const std::string& path, unsigned int w, unsigned int h){ this->path = path; this->w = w; this->h = h; fit = NULL; }

	// predicate function's(全て降順にする)
	static int cmp_w(const Data& l, const Data& r){
		return static_cast<int>(r.w) - static_cast<int>(l.w);
	}
	static int cmp_h(const Data& l, const Data& r){
		return static_cast<int>(r.h) - static_cast<int>(l.h);
	}
	static int cmp_max(const Data& l, const Data& r){
		return static_cast<int>(std::max(r.w, r.h)) - static_cast<int>(std::max(l.w, l.h));
	}
	static int cmp_min(const Data& l, const Data& r){
		return static_cast<int>(std::min(r.w, r.h)) - static_cast<int>(std::min(l.w, l.h));
	}
	static bool cmp_maxside(const Data& l, const Data& r){
		int diff;
		if((diff = cmp_max(l, r)) != 0)
			return (diff > 0)? false: true;
		if((diff = cmp_min(l, r)) != 0)
			return (diff > 0)? false: true;
		if((diff = cmp_h(l, r)) != 0)
			return (diff > 0)? false: true;
		if((diff = cmp_w(l, r)) != 0)
			return (diff > 0)? false: true;
		return false;
	}
public:
	std::string path;
	unsigned int w;
	unsigned int h;
	Node* fit;
};

typedef std::vector<Data> DataArray;

/**
 * @class Packer
 */
class Packer{
public:
	Packer();
	~Packer();
	bool pack(DataArray& data_array);
	inline unsigned int getW() const { return width; }
	inline unsigned int getH() const { return height; }
	inline unsigned int getPadding() const { return padding; }
	inline void setPadding(unsigned int padding){ this->padding = padding; }
	inline unsigned int calcActualSize(unsigned int size){ return size + padding * 2; }
	inline void setAligned(bool aligned){ this->aligned = aligned; }
	inline bool isAligned() const { return aligned; }
private:
	Node* findNode(const Node* node, const Data* data);
	Node* splitNode(Node* node, const Data* data);
	Node* growNode(const Data* data);
	Node* growRight(const Data* data);
	Node* growBottom(const Data* data);
private:
	Node* root;
	unsigned int width;
	unsigned int height;
	unsigned int padding;	// for pixel
	bool aligned;
};