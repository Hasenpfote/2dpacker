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
 * �p�b�N
 * @param[io] data_array: ���̓f�[�^
 */
bool Packer::pack(DataArray& data_array){
	// �\�[�g���s��
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
 * �m�[�h�̒T��
 * @param[i] root: �T���J�n�m�[�h
 * @param[i] data: �i�[�f�[�^
 * @return ���������ɍ��v�����m�[�h
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
 * �m�[�h�̕���
 * @param[i] node: �����Ώۃm�[�h
 * @param[i] data: �i�[�f�[�^
 * @return node�Ɠ���
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
 * �m�[�h���L����
 * @param[i] data: �i�[�f�[�^
 * @return ���������ɍ��v�����m�[�h
 * @note �m�[�h���L����ۂɂȂ�ׂ������`�ɋ߂Â��悤�ɂ���
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

	if(shouldGrowRight)	// �E�����ɍL����ׂ�
		return growRight(data);
	if(shouldGrowBottom)// �������ɍL����ׂ�
		return growBottom(data);
	if(canGrowRight)	// �E�����ɍL���邱�Ƃ��\
		return growRight(data);
	if(canGrowBottom)	// �������ɍL���邱�Ƃ��\
		return growBottom(data);
	return NULL;		// ���[�g�m�[�h�̑I�ʂɖ�肪����
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
	return NULL;	// �ʏ�͓��B���Ȃ�
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
	return NULL;	// �ʏ�͓��B���Ȃ�
}
