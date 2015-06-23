#include "KBinaryTreeNode.h"

KBinaryTreeNode::KBinaryTreeNode(VOID* data, int balance, KBinaryTreeNode* parent, KBinaryTreeNode* left, KBinaryTreeNode* right)
{
  m_pParent = parent;
  m_pRight = right;
  m_pLeft = left;
  m_nBalance = balance;         
  m_pData = data;
}

KBinaryTreeNode::~KBinaryTreeNode()
{
  if (m_pLeft != NULL)
    delete m_pLeft;
  m_pLeft = NULL;

  if (m_pRight != NULL)
    delete m_pRight;
  m_pRight = NULL;
}

BOOLEAN KBinaryTreeNode::RestructureInsert()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* item = this;

  while (!item->IsRoot())
  {
    // rule 1
    // Regel 1
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == 0)
    {
      item->m_pParent->m_nBalance = -1;
      item = item->m_pParent;
      continue;
    }
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == 0)
    {
      item->m_pParent->m_nBalance = 1;
      item = item->m_pParent;
      continue;
    }
    // rule 2
    // Regel 2
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == 1)
    {
      item->m_pParent->m_nBalance = 0;
      break;
    }
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == -1)
    {
      item->m_pParent->m_nBalance = 0;
      break;
    }
    // rule 3
    // Regel 3
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == -1)
    {
      if (item->m_nBalance == 1)
        item->m_pParent->DoubleRotationLeft();
      else
        item->m_pParent->RightRotation();
      break;
    }
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == 1)
    {
      if (item->m_nBalance == -1)
        item->m_pParent->DoubleRotationRight();
      else
        item->m_pParent->LeftRotation();
      break;
    }
  }

  bRet = TRUE;

  return bRet;
}

BOOLEAN KBinaryTreeNode::RestructureDelete()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* item = this;

  // Regel 1
  if (item->m_nBalance == 0 && item->m_pLeft == NULL) 
  {
    item->m_nBalance = 1;
    return TRUE;
  }
  if (item->m_nBalance == 0 && item->m_pRight == NULL) 
  {
    item->m_nBalance = -1;
    return TRUE;
  }
  // Regel 2
  if (item->m_nBalance == -1 && item->m_pLeft == NULL)
  {
    item->m_nBalance = 0;
  }
  if (item->m_nBalance == 1 && item->m_pRight == NULL)
  {
    item->m_nBalance = 0;
  }
  // Regel 3
  if (item->m_nBalance == -1 && item->m_pRight == NULL)
  {
    if (item->m_pLeft->m_nBalance == 1)
    {
      item->DoubleRotationLeft();
      item = item->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
    }
    else
    {
      item->RightRotation();
      item = item->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
    }
    if (item->m_nBalance == 1)
    {
      return TRUE;
    }
  }
  if (item->m_nBalance == 1 && item->m_pLeft == NULL)
  {
    if (item->m_pRight->m_nBalance == -1)
    {
      item->DoubleRotationRight();
      item = item->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
    }
    else
    {
      item->LeftRotation();
      item = item->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
    }
    if (item->m_nBalance == -1)
    {
      return TRUE;
    }
    //return true;
  }
  // Beginn des eigentlichen Aufstiegs
  while (!item->IsRoot())
  {
    // Regel 1
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == 0)
    {
      item->m_pParent->m_nBalance = 1;
      break;
    }
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == 0)
    {
      item->m_pParent->m_nBalance = -1;
      break;
    }
    // Regel 2
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == -1)
    {
      item->m_pParent->m_nBalance = 0;
      item = item->m_pParent;
      continue;
    }
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == 1)
    {
      item->m_pParent->m_nBalance = 0;
      item = item->m_pParent;
      continue;
    }
    // Regel 3
    if (item->IsRightSibling() && item->m_pParent->m_nBalance == -1)
    {
      if (item->m_pParent->m_pLeft->m_nBalance == 1)
      {
        item->m_pParent->DoubleRotationLeft();
        item = item->m_pParent->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
      }
      else
      {
        item->m_pParent->RightRotation();
        item = item->m_pParent->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
      }
      if (item->m_nBalance == 1)
      {
        return TRUE;
      }
      continue;
    }
    if (item->IsLeftSibling() && item->m_pParent->m_nBalance == 1)
    {
      if (item->m_pParent->m_pRight->m_nBalance == -1)
      {
        item->m_pParent->DoubleRotationRight();
        item = item->m_pParent->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
      }
      else
      {
        item->m_pParent->LeftRotation();
        item = item->m_pParent->m_pParent; // Zeiger sind durch die Rotation etwas verrutscht
      }
      if (item->m_nBalance == -1)
      {
        return TRUE;
      }
      continue;
    }
    // Never appears...
    break;
  }

  bRet = TRUE;
  

  return bRet;
}

KBinaryTreeNode* KBinaryTreeNode::SearchByNode(VOID* data, BTREE_COMPARE pCompare, ULONG dwCompareParam)
{
  KBinaryTreeNode* item = NULL;
  int         nCmp;

  if (pCompare != NULL)
  {
    nCmp = pCompare(m_pData, data, dwCompareParam);
  }
  else
  {
    if ((DWORD)m_pData > (DWORD)data)
    {
      nCmp = 1;
    }
    else
    {
      if ((DWORD)m_pData < (DWORD)data)
        nCmp = -1;
      else
        nCmp = 0;
    }
  }

  switch (nCmp)
  {
    case -1: 
         if (!m_pRight)
           item = NULL;
         else
           item = m_pRight->SearchByNode(data, pCompare, dwCompareParam);
         break;
    case 0:
         item = this;
         break;
    case 1:
         if (!m_pLeft)
           item = NULL;
         else
           item = m_pLeft->SearchByNode(data, pCompare, dwCompareParam);
         break;
  }
  

  return item;
}

BOOLEAN KBinaryTreeNode::IsRoot()
{
  return (m_pParent == NULL);
}

BOOLEAN KBinaryTreeNode::IsLeftSibling()
{
  return (!IsRoot() && m_pParent->m_pLeft == this);
}

BOOLEAN KBinaryTreeNode::IsRightSibling()
{
  return (!IsRoot() && m_pParent->m_pRight == this);
}

BOOLEAN KBinaryTreeNode::HasLeftSibling()
{
  return (!IsRoot() && IsRightSibling() && m_pParent->m_pLeft != NULL);
}
    
BOOLEAN KBinaryTreeNode::HasRightSibling()
{
  return (!IsRoot() && IsLeftSibling() && m_pParent->m_pRight != NULL);
}

LONG KBinaryTreeNode::GetDepthNode()
{
  LONG i = 0;

  if (m_pLeft != NULL)
    i = m_pLeft->GetDepthNode();
  if (m_pRight != NULL)
    i = max(i, m_pRight->GetDepthNode());

  return i+1;
}

LONG KBinaryTreeNode::GetLevel()
{
  KBinaryTreeNode* item = this;
  LONG        level = 0;

  while (item->m_pParent != NULL)
  {
    item = item->m_pParent;
    level++;
  }

  return level;
}

LONG KBinaryTreeNode::NodesInTreeByNode()
{
  int Nodes = 1;

  if (m_pLeft != NULL) 
      Nodes += m_pLeft->NodesInTreeByNode();
  if (m_pRight != NULL)
      Nodes += m_pRight->NodesInTreeByNode();

  return Nodes;
}

KBinaryTreeNode* KBinaryTreeNode::GetRootByNode()
{
  KBinaryTreeNode* pRoot = this;

  while (pRoot->m_pParent != NULL)
    pRoot = pRoot->m_pParent;

  return pRoot;
}

KBinaryTreeNode* KBinaryTreeNode::GetLeftSibling()
{
  KBinaryTreeNode* pLeftNode = NULL;

  if (!IsRoot() && !IsLeftSibling())
    pLeftNode = m_pParent->m_pLeft;

  return pLeftNode;
}

KBinaryTreeNode* KBinaryTreeNode::GetRightSibling()
{
  KBinaryTreeNode* pRightNode = NULL;

  if (!IsRoot() && !IsRightSibling())
    pRightNode = m_pParent->m_pRight;

  return pRightNode;
}

KBinaryTreeNode* KBinaryTreeNode::GetFirstNodeInOrder()
{
  KBinaryTreeNode* item = this;

  while (item->m_pLeft != NULL)
    item = item->m_pLeft;

  return item;
}

KBinaryTreeNode* KBinaryTreeNode::GetLastNodeInOrder()
{
  KBinaryTreeNode* item = this;

  while (item->m_pRight != NULL)
    item = item->m_pRight;

  return item;
}

KBinaryTreeNode* KBinaryTreeNode::GetPrevNodeInOrder()
{
  KBinaryTreeNode* item = NULL;
  KBinaryTreeNode* itemtmp;

  if (m_pLeft != NULL)
  {
    item = m_pLeft->GetLastNodeInOrder();
  }
  else
  {
    if (IsRightSibling())
    {
      item = m_pParent;
    }
    else
    {
      itemtmp = this;
      while (!itemtmp->IsRoot())
      {
        if (itemtmp->IsLeftSibling()) 
        {
          itemtmp = itemtmp->m_pParent;
        }
        else
        {
          item = itemtmp->m_pParent;
          break;
        }
      }
    }
  }

  return item;
}

KBinaryTreeNode* KBinaryTreeNode::GetNextNodeInOrder()
{
  KBinaryTreeNode* item = NULL;
  KBinaryTreeNode* itemtmp;

  if (m_pRight != NULL)
  {
    item = m_pRight->GetFirstNodeInOrder();
  }
  else
  {
    if (IsLeftSibling())
    {
      item = m_pParent;
    }
    else
    {
      itemtmp = this;
      while (!itemtmp->IsRoot())
      {
        if (itemtmp->IsRightSibling()) 
        {
          itemtmp = itemtmp->m_pParent;
        }
        else
        {
          item = itemtmp->m_pParent;
          break;
        }
      }
    }
  }
  
  return item;
}

KBinaryTreeNode* KBinaryTreeNode::GetInsertPosition(VOID* data, BTREE_COMPARE pCompare, ULONG dwCompareParam)
{
  KBinaryTreeNode* item = NULL;
  int         nCmp;

  if (pCompare != NULL)
  {
    nCmp = pCompare(m_pData, data, dwCompareParam);
  }
  else
  {
    if ((DWORD)m_pData > (DWORD)data)
    {
      nCmp = 1;
    }
    else
    {
      if ((DWORD)m_pData < (DWORD)data)
        nCmp = -1;
      else
        nCmp = 0;
    }
  }

  switch (nCmp)
  {
    case -1:
         if (!m_pRight)
           item = this;
         else
           item = m_pRight->GetInsertPosition(data, pCompare, dwCompareParam);
         break;
    case 0:
         item = 0;
         break;
    case 1:
         if (!m_pLeft)
           item = this;
         else
           item = m_pLeft->GetInsertPosition(data, pCompare, dwCompareParam);
         break;
  }

  return item;
}

BOOLEAN KBinaryTreeNode::LeftRotation()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* b;

//    ASSERT(Right != NULL);

  if (m_pRight != NULL)
  {
    b = m_pRight;
    if (!IsRoot())
    {
      if (IsLeftSibling())
        m_pParent->m_pLeft = b;
      else
        m_pParent->m_pRight = b;
      b->m_pParent = m_pParent;
    }
    else
    {
      b->m_pParent = NULL;
    }
  
    m_pRight = b->m_pLeft;
    b->m_pLeft = this;

    m_pParent = b;
    if (m_pRight != NULL)
      m_pRight->m_pParent = this;
  
    if (b->m_nBalance == 0)
    {
      m_nBalance = 1;
      b->m_nBalance = -1;
    }
    else
    {
      m_nBalance = 0;
      b->m_nBalance = 0;
    }

    bRet = TRUE;
  }

  return bRet;
}

BOOLEAN KBinaryTreeNode::RightRotation()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* b;

//  ASSERT(Left != NULL);
  
  if (m_pLeft != NULL) 
  {

    b = m_pLeft;
    if (!IsRoot())
    {
      if (IsLeftSibling())
        m_pParent->m_pLeft = b;
      else
        m_pParent->m_pRight = b;
      b->m_pParent = m_pParent;
    }
    else
    {
      b->m_pParent = NULL;
    }
  
    m_pLeft = b->m_pRight;
    b->m_pRight = this;

    m_pParent = b;
    if (m_pLeft != NULL)
      m_pLeft->m_pParent = this;

    if (b->m_nBalance == 0)
    {
      m_nBalance = -1;
      b->m_nBalance = 1;
    }
    else
    {
      m_nBalance = 0;
      b->m_nBalance = 0;
    }

    bRet = TRUE;
  }

  return bRet;
}

BOOLEAN KBinaryTreeNode::DoubleRotationLeft()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* b;
  KBinaryTreeNode* c;

//  ASSERT(Left != NULL && Left->Right != NULL);

  if (m_pLeft != NULL && m_pLeft->m_pRight != NULL) 
  {

    b = m_pLeft;
    c = m_pLeft->m_pRight;

    if (!IsRoot())
    {
      if (IsLeftSibling())
        m_pParent->m_pLeft = c;
      else
        m_pParent->m_pRight = c;
    }

    b->m_pRight = c->m_pLeft;
    m_pLeft = c->m_pRight;
    c->m_pLeft = b;
    c->m_pRight = this;

    c->m_pParent = m_pParent;
    m_pParent = c;
    b->m_pParent = c;
    if (b->m_pRight != NULL)
      b->m_pRight->m_pParent = b;
    if (m_pLeft != NULL)
      m_pLeft->m_pParent = this;

    switch (c->m_nBalance)
    {
      case -1:
           m_nBalance = 1;
           b->m_nBalance = 0;
           break;
      case 0:
           m_nBalance = 0;
           b->m_nBalance = 0;
           break;
      case 1:
           m_nBalance = 0;
           b->m_nBalance = -1;
           break;
    }
    c->m_nBalance = 0;

    bRet = TRUE;
  }

  return bRet;
}

BOOLEAN KBinaryTreeNode::DoubleRotationRight()
{
  BOOLEAN     bRet = FALSE;
  KBinaryTreeNode* b;
  KBinaryTreeNode* c;

//  ASSERT(Right != NULL && Right->Left != NULL);
  
  if (m_pRight != NULL && m_pRight->m_pLeft != NULL) 
  {

    b = m_pRight;
    c = m_pRight->m_pLeft;

    if (!IsRoot())
    {
      if (IsLeftSibling())
        m_pParent->m_pLeft = c;
      else
        m_pParent->m_pRight = c;
    }

    m_pRight = c->m_pLeft;
    b->m_pLeft = c->m_pRight;
    c->m_pLeft = this;
    c->m_pRight = b;

    c->m_pParent = m_pParent;
    m_pParent = c;
    b->m_pParent = c;
    if (m_pRight != NULL)
      m_pRight->m_pParent = this;
    if (b->m_pLeft != NULL)
      b->m_pLeft->m_pParent = b;

    switch (c->m_nBalance)
    {
      case -1:
           m_nBalance = 0;
           b->m_nBalance = 1;
           break;
      case 0:
           m_nBalance = 0;
           b->m_nBalance = 0;
           break;
      case 1:
           m_nBalance = -1;
           b->m_nBalance = 0;
           break;
    }
    c->m_nBalance = 0;

    bRet = TRUE;
  }

  return bRet;
}
