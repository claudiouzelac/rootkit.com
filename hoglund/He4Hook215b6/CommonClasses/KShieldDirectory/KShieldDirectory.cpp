#include "KShieldDirectory.h"
#include "../KDLinkedList/KDLinkedList.h"
#include "../KBinaryTree/KBinaryTree.h"
#include "../KLocker/KLocker.h"

KShieldDirectory::KShieldDirectory(KShieldDirectory* pParentDirectory)
{
  m_bOpen = FALSE;
  m_pDListMatchNameItems = NULL;
  m_KBTreeFullNameItems = NULL;
  m_pDirectoryInfo = NULL;
  m_pParentDirectory = pParentDirectory;
}

KShieldDirectory::~KShieldDirectory()
{
  Close();
}

BOOLEAN KShieldDirectory::Open(PWSTR pwszName, PVOID pUserContext)
{
  KLocker locker(&m_KSynchroObject);

  Close();

  m_pDirectoryInfo = CreateDirectoryInfo(pwszName, pUserContext);
  if (m_pDirectoryInfo != NULL)
  {
    m_KBTreeFullNameItems = new KBinaryTree((BTREE_COMPARE)_CompareFullNames);
    if (m_KBTreeFullNameItems != NULL)
    {
      m_pDListMatchNameItems = new KDLinkedList(NULL);
      if (m_pDListMatchNameItems != NULL)
      {
        m_bOpen = TRUE;
      }
      else
      {
        delete m_KBTreeFullNameItems;
        m_KBTreeFullNameItems = NULL;
        delete m_pDirectoryInfo;
        m_pDirectoryInfo = NULL;
      }
    }
    else
    {
      delete m_pDirectoryInfo;
      m_pDirectoryInfo = NULL;
    }
  }

  return m_bOpen;
}

void KShieldDirectory::Close()
{
  KLocker locker(&m_KSynchroObject);

  if (m_bOpen == TRUE)
  {
    if (m_KBTreeFullNameItems != NULL)
    {
      KShieldDirectory* pItem;
      while (m_KBTreeFullNameItems->IsEmpty() == FALSE)
      {
        pItem = (KShieldDirectory*)m_KBTreeFullNameItems->GetRootData();
        m_KBTreeFullNameItems->Delete(pItem, SDIR_SEARCH_BY_INFO);
        if (pItem != NULL)
          delete pItem;
      }
      
      delete m_KBTreeFullNameItems;
    }
    m_KBTreeFullNameItems = NULL;

    if (m_pDListMatchNameItems != NULL)
    {
      KDLinkedListItem* pItemStart;
      KShieldDirectory* pItem;
      while (m_pDListMatchNameItems->IsEmpty() == FALSE)
      {
        pItemStart = m_pDListMatchNameItems->GetHead();
        pItem = ((KShieldDirectory*)pItemStart->GetObject());
        m_pDListMatchNameItems->Remove(pItemStart);
        delete pItem;
      }
      delete m_pDListMatchNameItems;
    }
    m_pDListMatchNameItems = NULL;
    
    if (m_pDirectoryInfo != NULL)
      delete m_pDirectoryInfo;
    m_pDirectoryInfo = NULL;
    m_bOpen = FALSE;
  }
}

BOOLEAN KShieldDirectory::IsMatchedName(PWSTR pwszName, WCHAR Symbol)
{
  while (*pwszName != 0)
  {
    if (!(*pwszName ^ Symbol))
      return TRUE;
    ++pwszName;
  }

  return FALSE;
}

KShieldDirectory* KShieldDirectory::Find(PWSTR pwszName)
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN         bMatched = FALSE;

  if (m_bOpen == FALSE || pwszName == NULL)
    return NULL;

  KShieldDirectory* pItem = NULL;

  _wcslwr(pwszName);

  bMatched = IsMatchedName(pwszName, L'*') || IsMatchedName(pwszName, L'?');
  if (bMatched == TRUE)
  {
    KDLinkedListItem* pItemStart = m_pDListMatchNameItems->GetHead();
    KShieldDirectory::PDIRECTORY_INFO pDirInfoStart;
    int nCmpRes;
    while (pItemStart != NULL)
    {
      pItem = (KShieldDirectory*)pItemStart->GetObject();
      pDirInfoStart = pItem->m_pDirectoryInfo;
      nCmpRes = wcscmp(pDirInfoStart->m_pwszName, pwszName);
      if (nCmpRes == 0)
        break;
      pItemStart = pItemStart->GetNext();
    }
    if (pItemStart == NULL)
      pItem = NULL;
  }
  else
  {
    pItem = (KShieldDirectory*)m_KBTreeFullNameItems->SearchData(pwszName, SDIR_SEARCH_BY_NAME);
  }

  return pItem;
}

#ifdef __TEST_WIN32
BOOLEAN MatchW(WCHAR *string, WCHAR *pattern)
{
  for (; L'*'^*pattern; ++pattern, ++string) 
  {
    if (!*string)
      return (!*pattern);
    if (towupper(*string)^towupper(*pattern) && L'?'^*pattern)
      return FALSE;
  }
  /* two-line patch to prevent *too* much recursiveness: */
  while (L'*' == pattern[1])
    ++pattern;

  do
  {
    if (MatchW(string, pattern + 1))
      return TRUE;
  }
  while (*string++);

  return FALSE;
}
#endif //__TEST_WIN32

KShieldDirectory* KShieldDirectory::FindMatch(PWSTR pwszName)
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN         bMatched = FALSE;

  if (m_bOpen == FALSE || pwszName == NULL)
    return NULL;

  KShieldDirectory* pItem = NULL;

  _wcslwr(pwszName);

  pItem = (KShieldDirectory*)m_KBTreeFullNameItems->SearchData(pwszName, SDIR_SEARCH_BY_NAME);
  if (pItem == NULL)
  {
    KDLinkedListItem* pItemStart = m_pDListMatchNameItems->GetHead();
    KShieldDirectory::PDIRECTORY_INFO pDirInfoStart;
    BOOLEAN bCmpRes;
    while (pItemStart != NULL)
    {
      pItem = (KShieldDirectory*)pItemStart->GetObject();
      pDirInfoStart = pItem->m_pDirectoryInfo;
      bCmpRes = MatchW(pwszName, pDirInfoStart->m_pwszName);
      if (bCmpRes == TRUE)
        break;
      pItemStart = pItemStart->GetNext();
    }
    if (pItemStart == NULL)
      pItem = NULL;
  }

  return pItem;
}

KShieldDirectory* KShieldDirectory::Add(PWSTR pwszName, PVOID pUserContext)
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN         bMatched = FALSE;
  BOOLEAN         bAdded = FALSE;

  if (m_bOpen == FALSE || pwszName == NULL)
    return NULL;

  KShieldDirectory* pItem = new KShieldDirectory(this);
  if (pItem != NULL)
  {
    _wcslwr(pwszName);
    if (pItem->Open(pwszName, pUserContext))
    {
      bMatched = IsMatchedName(pwszName, L'*') || IsMatchedName(pwszName, L'?');
      if (bMatched == TRUE)
      {
        if (m_pDListMatchNameItems->IsEmpty() == TRUE)
        {
          bAdded = m_pDListMatchNameItems->AddHeadObject(pItem);
        }
        else
        {
          KDLinkedListItem* pItemStart = m_pDListMatchNameItems->GetHead();
          KShieldDirectory::PDIRECTORY_INFO pDirInfoStart;
          KShieldDirectory::PDIRECTORY_INFO pDirInfo = pItem->m_pDirectoryInfo;
          int nCmpRes;
          while (pItemStart != NULL)
          {
            pDirInfoStart = ((KShieldDirectory*)pItemStart->GetObject())->m_pDirectoryInfo;
            nCmpRes = wcscmp(pDirInfoStart->m_pwszName, pDirInfo->m_pwszName);
            if (nCmpRes < 0)
            {
              bAdded = m_pDListMatchNameItems->AddObject(pItem, pItemStart->GetPrev());
              break;
            }
            else
            {
              if (nCmpRes == 0)
                break;
            }

            pItemStart = pItemStart->GetNext();
          }
          if (pItemStart == NULL)
            bAdded = m_pDListMatchNameItems->AddObject(pItem, m_pDListMatchNameItems->GetTail());
        }
      }
      else
      {
        if (m_KBTreeFullNameItems->Insert(pItem, SDIR_SEARCH_BY_INFO) != NULL)
          bAdded = TRUE;
      }
    }

    if (bAdded == FALSE)
    {
      delete pItem;
      pItem = NULL;
    }
    else
    {
      ++(m_pDirectoryInfo->m_dwChildItemsCount);
    }
  }

  return pItem;
}

BOOLEAN KShieldDirectory::Remove(PWSTR pwszName)
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN bRes = TRUE;
  KShieldDirectory* pItem = Find(pwszName);
  if (pItem != NULL)
  {
    if (pItem->IsEmpty() == TRUE)
    {
      BOOLEAN bMatched = IsMatchedName(pwszName, L'*') || IsMatchedName(pwszName, L'?');
      if (bMatched == TRUE)
        m_pDListMatchNameItems->Remove(m_pDListMatchNameItems->Find(pItem));
      else
        m_KBTreeFullNameItems->Delete(pItem, SDIR_SEARCH_BY_INFO);

      --(m_pDirectoryInfo->m_dwChildItemsCount);
      delete pItem;
    }
    else
    {
      bRes = FALSE;
    }
  }

  return bRes;
}

BOOLEAN KShieldDirectory::IsEmpty()
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN bEmpty = TRUE;

  if (m_bOpen == TRUE)
  {
    bEmpty = m_KBTreeFullNameItems->IsEmpty();
    bEmpty = bEmpty && m_pDListMatchNameItems->IsEmpty();
  }

  return bEmpty;
}

KShieldDirectory::PDIRECTORY_INFO KShieldDirectory::CreateDirectoryInfo(PWSTR pwszName, PVOID pUserContext)
{
  if (pwszName == NULL)
    return NULL;

  KShieldDirectory::PDIRECTORY_INFO pDirInfo = new DIRECTORY_INFO;
  if (pDirInfo != NULL)
  {
    pDirInfo->m_pUserContext = pUserContext;
    pDirInfo->m_nSizeName = wcslen(pwszName) * sizeof(WCHAR) + sizeof(WCHAR);
    if (pDirInfo->m_nSizeName > sizeof(WCHAR))
    {
      pDirInfo->m_pwszName = new WCHAR[pDirInfo->m_nSizeName/sizeof(WCHAR)];
      if (pDirInfo->m_pwszName != NULL)
      {
        wcscpy(pDirInfo->m_pwszName, pwszName);
        pDirInfo->m_bCompareRest = IsMatchedName(pwszName, L'*');
      }
      else
      {
        delete pDirInfo;
        pDirInfo = NULL;
      }
    }
    else
    {
      delete pDirInfo;
      pDirInfo = NULL;
    }
  }

  return pDirInfo;
}

int KShieldDirectory::_CompareFullNames(PVOID* pData1, PVOID* pData2, ULONG dwParam)
{
  switch (dwParam)
  {
    case SDIR_SEARCH_BY_INFO:
    {
         KShieldDirectory* pDir1 = (KShieldDirectory*)pData1; 
         KShieldDirectory* pDir2 = (KShieldDirectory*)pData2;
         return KShieldDirectory::CompareDirInfo(pDir1->m_pDirectoryInfo, pDir2->m_pDirectoryInfo);
    }
    case SDIR_SEARCH_BY_NAME:
    {
         KShieldDirectory* pDir = (KShieldDirectory*)pData1; 
         PWSTR pwszName = (PWSTR)pData2;
         return KShieldDirectory::CompareNameAndDirInfo(pDir->m_pDirectoryInfo, pwszName);
    }
  }
  return 0;
}

int KShieldDirectory::CompareDirInfo(KShieldDirectory::PDIRECTORY_INFO pDirInfo1, KShieldDirectory::PDIRECTORY_INFO pDirInfo2)
{
  int nCmp = 0;
  BOOLEAN bValid1, bValid2;

  bValid1 = (pDirInfo1 != NULL);
  bValid2 = (pDirInfo2 != NULL);

  if (!bValid1 && !bValid2)
  {
    nCmp = 0;
  }
  else
  {
    if (bValid1 && !bValid2)
    {
      nCmp = 1;
    }
    else
    {
      if (!bValid1 && bValid2)
      {
        nCmp = -1;
      }
      else
      {
        nCmp = wcscmp(pDirInfo1->m_pwszName, pDirInfo2->m_pwszName);
      }
    }
  }

  return nCmp;
}

int KShieldDirectory::CompareNameAndDirInfo(PDIRECTORY_INFO pDirInfo, PWSTR pwszName)
{
  int nCmp = 0;
  BOOLEAN bValid1, bValid2;

  bValid1 = (pwszName != NULL);
  bValid2 = (pDirInfo != NULL);

  if (!bValid1 && !bValid2)
  {
    nCmp = 0;
  }
  else
  {
    if (bValid1 && !bValid2)
    {
      nCmp = 1;
    }
    else
    {
      if (!bValid1 && bValid2)
      {
        nCmp = -1;
      }
      else
      {        
        nCmp = wcscmp(pDirInfo->m_pwszName, pwszName);
      }
    }
  }

  return nCmp;
}