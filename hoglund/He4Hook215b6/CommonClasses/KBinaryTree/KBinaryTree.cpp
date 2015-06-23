#include "KBinaryTree.h"
#include "../KLocker/KLocker.h"

KBinaryTree::KBinaryTree(BTREE_COMPARE pCompare)
{
  m_pRootTree = NULL;
  m_pCompare = pCompare;
}

KBinaryTree::~KBinaryTree()
{
  if (m_pRootTree != NULL)
    delete m_pRootTree;
  m_pRootTree = NULL;
}

BOOLEAN KBinaryTree::IsEmpty()
{
  BOOLEAN bEmpty = TRUE;
  KLocker locker(&m_KSynchroObject);

  bEmpty = (BOOLEAN) (m_pRootTree == NULL);

  return bEmpty;
}

LONG KBinaryTree::GetDepth()
{
  LONG  nDepth = 0;
  KLocker locker(&m_KSynchroObject);

  if (m_pRootTree != NULL)
    nDepth = m_pRootTree->GetDepthNode();

  return nDepth;
}

KBinaryTreeNode* KBinaryTree::Insert(VOID* data, ULONG dwCompareParam)
{
  KBinaryTreeNode* pNode = NULL;
  KBinaryTreeNode* pNewNode = NULL;
  KLocker locker(&m_KSynchroObject);


  if (IsEmpty())
  {
    m_pRootTree = new KBinaryTreeNode(data, 0, NULL, NULL, NULL);
    pNewNode = m_pRootTree;
  }
  else
  {
    pNode = m_pRootTree->GetInsertPosition(data, m_pCompare, dwCompareParam);
    if (pNode != NULL)
    {
      pNewNode = new KBinaryTreeNode(data, 0, pNode, NULL, NULL);
      if (pNewNode != NULL)
      {
        if (m_pCompare != NULL)
        {
          if (m_pCompare(pNode->m_pData, data, dwCompareParam) == -1)
            pNode->m_pRight = pNewNode;
          else
            pNode->m_pLeft = pNewNode;
        }
        else
        {
          if ((DWORD)pNode->m_pData >= (DWORD)data)
          {
            pNode->m_pLeft = pNewNode;
          }
          else
          {
            if ((DWORD)pNode->m_pData < (DWORD)data)
              pNode->m_pRight = pNewNode;
          }
        }
        pNewNode->RestructureInsert();
        m_pRootTree = m_pRootTree->GetRootByNode();
      }
    }
  }

  return pNewNode;
}

BOOLEAN KBinaryTree::Delete(VOID* data, ULONG dwCompareParam)
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* item;
  KBinaryTreeNode* startitem;
  KBinaryTreeNode* y;
  KBinaryTreeNode* z;
  KLocker locker(&m_KSynchroObject);


  if (!IsEmpty())
  {
    item = m_pRootTree->SearchByNode(data, m_pCompare, dwCompareParam);
    if (item != NULL)
    {
      // if we want to delete the root item, we have to do some extra
      // operation the preserve some pointers...
      if (item == m_pRootTree)
      {
        // the root is the only one node in the tree
        if (item->m_pLeft == NULL && item->m_pRight == NULL)
        {
          delete m_pRootTree;
          m_pRootTree = NULL;
          return TRUE;
        }
      }

      // start node for restructuration
      startitem = NULL;
      // node to delete has no children
      if (item->m_pLeft == NULL && item->m_pRight == NULL)
      {
        if (item->IsLeftSibling())
          item->m_pParent->m_pLeft = NULL;
        else
          item->m_pParent->m_pRight = NULL;
        startitem = item->m_pParent;
        delete item;
        item = NULL;
      }
      // node to delete has only right son
      if (item != NULL && item->m_pLeft == NULL && item->m_pRight != NULL)
      {
        item->m_pData = item->m_pRight->m_pData;
        item->m_pRight->m_pData = NULL;

        delete item->m_pRight;

        item->m_pRight = NULL;
        startitem = item;
      }
      // node to delete has only left son
      if (item != NULL && item->m_pLeft != NULL && item->m_pRight == NULL)
      {
        item->m_pData = item->m_pLeft->m_pData;
        item->m_pLeft->m_pData = NULL;

        delete item->m_pLeft;

        item->m_pLeft = NULL;
        startitem = item;
      }
      // node to delete has both sons
      if (item != NULL && item->m_pLeft != NULL && item->m_pRight != NULL)
      {
        y = item->m_pLeft->GetLastNodeInOrder();
        z = y->m_pLeft;

        item->m_pData = y->m_pData;
        y->m_pData = NULL;
        /*
        if (y->IsLeftSibling())
            y->Parent->Left = z;
        else
            y->Parent->Right = z;
        */
        if (z != NULL)
        {
          y->m_pData = z->m_pData;
          z->m_pData = NULL;
          y->m_pLeft = NULL;
          delete z;
          startitem = y;
        }
        else
        {
          if (y->IsLeftSibling())
             y->m_pParent->m_pLeft = NULL;
          else
             y->m_pParent->m_pRight = NULL;
          startitem = y->m_pParent;
          delete y;
        }
      }

      if (startitem != NULL)
        startitem->RestructureDelete();

      m_pRootTree = m_pRootTree->GetRootByNode();
      
      bRet = TRUE;
    }
  }

  return bRet;
}

KBinaryTreeNode* KBinaryTree::Search(VOID* data, ULONG dwCompareParam)
{
  KLocker locker(&m_KSynchroObject);
  KBinaryTreeNode* pNode = GetRoot();

  if (pNode != NULL)
    pNode = pNode->SearchByNode(data, m_pCompare, dwCompareParam);

  return pNode;
}

VOID* KBinaryTree::SearchData(VOID* data, ULONG dwCompareParam)
{
  KLocker locker(&m_KSynchroObject);
  KBinaryTreeNode* pNode = GetRoot();
  VOID*       pData = NULL;

  if (pNode != NULL)
  {  
    pNode = pNode->SearchByNode(data, m_pCompare, dwCompareParam);
    if (pNode != NULL)
      pData = pNode->m_pData;
  }

  return pData;
}

KBinaryTreeNode* KBinaryTree::GetRoot()
{
  KLocker locker(&m_KSynchroObject);
  return m_pRootTree;
}

VOID* KBinaryTree::GetRootData()
{
  KLocker locker(&m_KSynchroObject);
  KBinaryTreeNode* pNode;
  VOID*       pData = NULL;

  pNode = m_pRootTree;
  if (pNode != NULL)
    pData = pNode->m_pData;

  return pData;
}

LONG KBinaryTree::NodesInTree()
{
  KLocker locker(&m_KSynchroObject);
  LONG nNodes = 0;

  if (m_pRootTree != NULL)
    nNodes = m_pRootTree->NodesInTreeByNode();

  return nNodes;
}

BOOLEAN KBinaryTree::DeleteAll()
{
  KLocker locker(&m_KSynchroObject);
  BOOLEAN bRet = TRUE;

  if (m_pRootTree != NULL)
  {
    delete m_pRootTree;
    m_pRootTree = NULL;
  }

  return bRet;
}
