#include <iostream>
#include <algorithm>
#include "packer.h"

/**
 * ceiling power of two
 */
static unsigned int clp2(unsigned int x){
	x--;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
	return x + 1;
}

/**
 * floor power of two
 */
static unsigned int flp2(unsigned int x){
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
	return x - (x >> 1);
}

static bool isPowerOfTwo(unsigned int x){
	// assert x > 0
	return !(x & (x - 1));
}

////////////////////////////////////////////////////////////////////////////////
// Node
////////////////////////////////////////////////////////////////////////////////
Node::Node(){
	sibling = NULL;
	right = NULL;
	bottom = NULL;
	use = false;
}

Node::~Node(){
	if(sibling)
		delete sibling;
	if(right)
		delete right;
	if(bottom)
		delete bottom;
}

////////////////////////////////////////////////////////////////////////////////
// Packer
////////////////////////////////////////////////////////////////////////////////
/**
 * constructor
 */
Packer::Packer(){
	root = NULL;
	width = 0;
	height = 0;
	padding = 0;
	aligned = false;
}

/**
 * destructor
 */
Packer::~Packer(){
	if(root)
		delete root;
}

/**
 * パック
 * @param[io] data_array: 入力データ
 */
bool Packer::pack(DataArray& data_array){
	// ソートを行う
	std::sort(data_array.begin(), data_array.end(), Data::cmp_maxside);

	try{
		root = new Node();
	}
	catch(std::bad_alloc& e){
		std::cerr << "could not allocate memory." << std::endl;
		return false;
	}
	const unsigned int w = calcActualSize(data_array[0].w);
	const unsigned int h = calcActualSize(data_array[0].h);
	root->rect.set(0, 0, w, h);
	width = w;
	height = h;

	Node* node;
	DataArray::iterator it = data_array.begin();
	while(it != data_array.end()){
		if(node = findNode(root, &(*it)))
			it->fit = splitNode(node, &(*it));
		else
			it->fit = growNode(&(*it));

		if(!it->fit){
			std::cerr << "this node does not fit." << std::endl;
		}
		it++;
	}

	if(aligned){
		width = clp2(width);
		height = clp2(height);
	}
	return true;
}

/**
 * ノードの探索
 * @param[i] root: 探索開始ノード
 * @param[i] data: 格納データ
 * @return 検索条件に合致したノード
 */
Node* Packer::findNode(const Node* node, const Data* data){
	Node* target = NULL;
	const unsigned int w = calcActualSize(data->w);
	const unsigned int h = calcActualSize(data->h);
	if(node->rect.contains(w, h)){
		if(node->use == false)
			return const_cast<Node*>(node);
		if(node->right)
			if(target = findNode(node->right, data))
				return target;
		if(node->bottom)
			if(target = findNode(node->bottom, data))
				return target;
	}
	if(node->sibling)
		if(target = findNode(node->sibling, data))
			return target;
	return NULL;
}

/**
 * ノードの分割
 * @param[i] node: 分割対象ノード
 * @param[i] data: 格納データ
 * @return nodeと同じ
 */
Node* Packer::splitNode(Node* node, const Data* data){
	node->use = true;
	const unsigned int w = calcActualSize(data->w);
	const unsigned int h = calcActualSize(data->h);
	if(w < node->rect.getW()){
		try{
			node->right = new Node();
		}
		catch(std::bad_alloc& e){
			std::cerr << "could not allocate memory." << std::endl;
			return node;
		}
		node->right->rect.set(node->rect.getX() + w, node->rect.getY(), node->rect.getW() - w, h);
	}
	if(h < node->rect.getH()){
		try{
			node->bottom = new Node();
		}
		catch(std::bad_alloc& e){
			std::cerr << "could not allocate memory." << std::endl;
			return node;
		}
		node->bottom->rect.set(node->rect.getX(), node->rect.getY() + h, node->rect.getW(), node->rect.getH() - h);
	}
	return node;
}

/**
 * ノードを広げる
 * @param[i] data: 格納データ
 * @return 検索条件に合致したノード
 * @note ノードを広げる際になるべく正方形に近づくようにする
 */
Node* Packer::growNode(const Data* data){
	const unsigned int w = calcActualSize(data->w);
	const unsigned int h = calcActualSize(data->h);
	const bool canGrowRight = (h <= height);
	const bool canGrowBottom = (w <= width);

	if(aligned && canGrowRight && canGrowBottom){
		const unsigned int ra = clp2(width + w) * clp2(height);
		const unsigned int la = clp2(width) * clp2(height + h);
		if(ra < la)
			return growRight(data);
		else
		if(la < ra)
			return growBottom(data);
	}

	const bool shouldGrowRight = canGrowRight && ((width + w) <= height);
	const bool shouldGrowBottom = canGrowBottom && ((height + h) <= width);

	if(shouldGrowRight)	// 右方向に広げるべき
		return growRight(data);
	if(shouldGrowBottom)// 下方向に広げるべき
		return growBottom(data);
	if(canGrowRight)	// 右方向に広げることが可能
		return growRight(data);
	if(canGrowBottom)	// 下方向に広げることが可能
		return growBottom(data);
	return NULL;		// ルートノードの選別に問題がある
}

Node* Packer::growRight(const Data* data){
	Node* node = root;
	while(node->sibling){
		node = node->sibling;
	}

	try{
		node->sibling = new Node();
	}
	catch(std::bad_alloc& e){
		std::cerr << "could not allocate memory." << std::endl;
		return NULL;
	}
	const unsigned int w = calcActualSize(data->w);
	node->sibling->rect.set(width, 0, w, height);
	width += w;

	if(node = findNode(root, data))
		return splitNode(node, data);
	return NULL;	// 通常は到達しない
}

Node* Packer::growBottom(const Data* data){
	Node* node = root;
	while(node->sibling){
		node = node->sibling;
	}

	try{
		node->sibling = new Node();
	}
	catch(std::bad_alloc& e){
		std::cerr << "could not allocate memory." << std::endl;
		return NULL;
	}
	const unsigned int h = calcActualSize(data->h);
	node->sibling->rect.set(0, height, width, h);
	height += h;

	if(node = findNode(root, data))
		return splitNode(node, data);
	return NULL;	// 通常は到達しない
}
