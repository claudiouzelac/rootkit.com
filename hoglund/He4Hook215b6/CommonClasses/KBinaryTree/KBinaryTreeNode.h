#ifndef __KBINARY_TREE_NODE_H
 #define __KBINARY_TREE_NODE_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
 
 #include "../Include/KNew.h"
 #include "../Include/KTypes.h"
#else
 #include <windows.h>
#endif //__TEST_WIN32


class KBinaryTree;
class KBinaryTreeNode;
//*******************************************************************//

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


typedef int (*BTREE_COMPARE)(VOID*, VOID*, ULONG);

class KBinaryTreeNode
{
   friend class KBinaryTree;
  public:
   explicit
   KBinaryTreeNode(VOID* data, int balance = 0, KBinaryTreeNode* parent = NULL, KBinaryTreeNode* left = NULL, KBinaryTreeNode* right = NULL);
   virtual ~KBinaryTreeNode();

  protected:
   BOOLEAN             RestructureInsert();
   BOOLEAN             RestructureDelete();
   KBinaryTreeNode*    SearchByNode(VOID* data, BTREE_COMPARE pCompare, ULONG dwCompareParam);
   BOOLEAN             IsRoot();
   BOOLEAN             IsLeftSibling();
   BOOLEAN             IsRightSibling();
   BOOLEAN             HasLeftSibling();
   BOOLEAN             HasRightSibling();
   LONG                GetDepthNode();
   LONG                GetLevel();
   LONG                NodesInTreeByNode();
   KBinaryTreeNode*    GetRootByNode();
   KBinaryTreeNode*    GetLeftSibling();
   KBinaryTreeNode*    GetRightSibling();
   KBinaryTreeNode*    GetFirstNodeInOrder();
   KBinaryTreeNode*    GetLastNodeInOrder();
   KBinaryTreeNode*    GetPrevNodeInOrder();
   KBinaryTreeNode*    GetNextNodeInOrder();
   KBinaryTreeNode*    GetInsertPosition(VOID* data, BTREE_COMPARE pCompare, ULONG dwCompareParam);
   BOOLEAN             LeftRotation();
   BOOLEAN             RightRotation();
   BOOLEAN             DoubleRotationLeft();
   BOOLEAN             DoubleRotationRight();

  private:
   KBinaryTreeNode(const KBinaryTreeNode&);
   KBinaryTreeNode& operator=(const KBinaryTreeNode& right);

  protected:
   KBinaryTreeNode*    m_pParent;
   KBinaryTreeNode*    m_pRight;
   KBinaryTreeNode*    m_pLeft;

   int                 m_nBalance;
   void*               m_pData;

  private:
};

#endif //__KBINARY_TREE_NODE_H

