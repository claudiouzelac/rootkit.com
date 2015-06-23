#ifndef __KBINARY_TREE_H
 #define __KBINARY_TREE_H

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

#include "../KSpinSynchroObject/KSpinSynchroObject.h"
//#include "../KNativeSynchroObject/KNativeSynchroObject.h"
#include "KBinaryTreeNode.h"

class KBinaryTree;
//*******************************************************************//

class KBinaryTree
{
  public:
   explicit
   KBinaryTree(BTREE_COMPARE pCompare = NULL);
   virtual ~KBinaryTree();

   BOOLEAN                IsEmpty();
   LONG                   GetDepth();
   KBinaryTreeNode*       Insert(VOID* data, ULONG dwCompareParam);
   BOOLEAN                Delete(VOID* data, ULONG dwCompareParam);
   KBinaryTreeNode*       Search(VOID* data, ULONG dwCompareParam);
   VOID*                  SearchData(VOID* data, ULONG dwCompareParam);
   KBinaryTreeNode*       GetRoot();
   VOID*                  GetRootData();
   LONG                   NodesInTree();
   BOOLEAN                DeleteAll();

  protected:

  private:
   KBinaryTree(const KBinaryTree&);
   KBinaryTree& operator=(const KBinaryTree& right);

  protected:
   KBinaryTreeNode*       m_pRootTree;
   BTREE_COMPARE          m_pCompare;

   KSpinSynchroObject     m_KSynchroObject;
   //KNativeSynchroObject   m_KSynchroObject;
};

#endif //__KBINARY_TREE_H