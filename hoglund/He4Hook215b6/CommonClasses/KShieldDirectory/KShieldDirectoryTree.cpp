#include "KShieldDirectoryTree.h"
#include "../KLocker/KLocker.h"

#ifdef __TEST_WIN32
extern BOOLEAN MatchW(WCHAR *string, WCHAR *pattern);

PWSTR _skipdelimit(PWSTR pwszString, PWSTR pwszDelimit)
{
  PWSTR pwszDelimitTmp;
  
  pwszDelimitTmp = pwszDelimit;
  while (*pwszDelimitTmp != 0)
  {
    while (!(*pwszString ^ *pwszDelimitTmp))
    {
      ++pwszString;
      pwszDelimitTmp = pwszDelimit-1;
      break;
    }
    ++pwszDelimitTmp;
  }

  return pwszString;
}

PWSTR _nextdelimit(PWSTR pwszString, PWSTR pwszDelimit)
{
  PWSTR pwszLast = pwszString;

  PWSTR pwszDelimitTmp = pwszDelimit;
  while (*pwszLast != 0)
  {
    if (*pwszDelimitTmp == 0)
    {
      pwszDelimitTmp = pwszDelimit;
      ++pwszLast;
    }
    if (!(*pwszLast ^ *pwszDelimitTmp))
      break;
    ++pwszDelimitTmp;
  }
  
  return pwszLast;
}

PWSTR _wcstok(PWSTR pwszString, PWSTR pwszDelimit, PWSTR* ppwszLast)
{
  if (pwszString == NULL)
    pwszString = *ppwszLast;

//  PWSTR pwszDelimitTmp;

/*  pwszDelimitTmp = pwszDelimit;
  while (*pwszDelimitTmp != 0)
  {
    while (!(*pwszString ^ *pwszDelimitTmp))
    {
      ++pwszString;
      pwszDelimitTmp = pwszDelimit-1;
      break;
    }
    ++pwszDelimitTmp;
  }
*/
  pwszString = _skipdelimit(pwszString, pwszDelimit);

//  PWSTR pwszLast = pwszString;
  PWSTR pwszLast = _nextdelimit(pwszString, pwszDelimit);
  
  if (*pwszLast != 0)
  {
    *pwszLast = 0;
    ++pwszLast;
  }
  *ppwszLast = pwszLast;

  /*
  pwszDelimitTmp = pwszDelimit;
  while (*pwszLast != 0)
  {
    if (*pwszDelimitTmp == 0)
    {
      pwszDelimitTmp = pwszDelimit;
      ++pwszLast;
    }
    if (!(*pwszLast ^ *pwszDelimitTmp))
    {
      if (*pwszLast != 0)
      {
        *pwszLast = 0;
        ++pwszLast;
      }
      break;
    }
    ++pwszDelimitTmp;
  }
  *ppwszLast = pwszLast;
  */

  if (*pwszString != 0)
    return pwszString;
  return NULL;
}
#endif //__TEST_WIN32

KShieldDirectoryTree::KShieldDirectoryTree()
{
  m_pRoot = NULL;
}

KShieldDirectoryTree::~KShieldDirectoryTree()
{
  m_KSynchroObject.Lock(0);

  if (m_pRoot != NULL)
    delete m_pRoot;
  m_pRoot = NULL;

  m_KSynchroObject.UnLock(0);
}

BOOLEAN KShieldDirectoryTree::Add(PWSTR pwszFullPath, PVOID pUserContext)
{
  KLocker locker(&m_KSynchroObject);

  if (pwszFullPath == NULL)
    return FALSE;

  int nPathLen = wcslen(pwszFullPath);
  int nSizeToken;
  PWSTR pwszFullPathSave = new WCHAR[nPathLen + sizeof(WCHAR)];
  if (pwszFullPathSave == NULL)
    return FALSE;
  wcscpy(pwszFullPathSave, pwszFullPath);
  PWSTR pwszEndPath = pwszFullPathSave + nPathLen;

  BOOLEAN bRes = FALSE;
  
  PWSTR pwszLast;
  //PWSTR pwszToken = _wcstok(pwszFullPathSave, L"\\/");
  PWSTR pwszToken = _wcstok(pwszFullPathSave, L"\\/", &pwszLast);

  if (pwszToken != NULL && m_pRoot == NULL)
  {
    m_pRoot = new KShieldDirectory(NULL);
    if (m_pRoot != NULL)
    {
      if (m_pRoot->Open(L"Root", NULL) != TRUE)
      {
        pwszToken = NULL;
        delete m_pRoot;
        m_pRoot = NULL;
      }
    }
    else
    {
      pwszToken = NULL;
    }
  }

  KShieldDirectory* pCurrentDir = m_pRoot;
  KShieldDirectory* pItem;
  PVOID pUserContextTmp;
  while (pwszToken != NULL)
  {
    pItem = pCurrentDir->Find(pwszToken);
    if (pItem == NULL)
    {
      pItem = pCurrentDir->Add(pwszToken, NULL);
      if (pItem == NULL)
        break;
    }
    nSizeToken = wcslen(pwszToken);
    if ((pwszToken+nSizeToken) != pwszEndPath)
      pUserContextTmp = NULL;
    else
      pUserContextTmp = pUserContext;

    if (pItem->m_pDirectoryInfo != NULL && pItem->m_pDirectoryInfo->m_pUserContext == NULL)
    {
      pItem->m_pDirectoryInfo->m_pUserContext = pUserContextTmp;
      if ((pwszToken+nSizeToken) == pwszEndPath)
      {
        if (pCurrentDir->m_pDirectoryInfo != NULL && pUserContextTmp != NULL)
          ++(pCurrentDir->m_pDirectoryInfo->m_dwUserCount);
        bRes = TRUE;
      }
    }

    pCurrentDir = pItem;

    pwszToken = _wcstok(NULL, L"\\/", &pwszLast);
  }

  delete[] pwszFullPathSave;

  return bRes;
}

BOOLEAN KShieldDirectoryTree::Remove(PWSTR pwszFullPath, PVOID* ppUserContext)
{
  KLocker locker(&m_KSynchroObject);

  BOOLEAN bRes = FALSE;
  KShieldDirectory* pItem = FindFrom(m_pRoot, pwszFullPath);
  if (pItem != NULL && pItem->m_pDirectoryInfo != NULL)
  {
    if (ppUserContext != NULL)
      *ppUserContext = pItem->m_pDirectoryInfo->m_pUserContext;

    if (pItem->m_pDirectoryInfo->m_pUserContext != NULL)
    {
      KShieldDirectory* pItemParent = pItem->m_pParentDirectory;
      while (pItemParent != NULL && pItem->m_pDirectoryInfo != NULL)
      {
        if (pItem->IsEmpty() == TRUE)
        {
          if (bRes == FALSE || pItem->m_pDirectoryInfo->m_pUserContext == NULL)
          {
            pItemParent->Remove(pItem->m_pDirectoryInfo->m_pwszName);
            if (bRes == FALSE)
              --(pItemParent->m_pDirectoryInfo->m_dwUserCount);
            bRes = TRUE;
          }
          else
          {
            break;
          }
        }
        else
        {
          if (bRes == FALSE)
          {
            pItem->m_pDirectoryInfo->m_pUserContext = NULL;
            --(pItemParent->m_pDirectoryInfo->m_dwUserCount);
            bRes = TRUE;
          }
          break;
        }
        pItem = pItemParent;
        pItemParent = pItem->m_pParentDirectory;
      }
    }
  }

  return bRes;
}

KShieldDirectory* KShieldDirectoryTree::Find(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext)
{
  KLocker locker(&m_KSynchroObject);

  KShieldDirectory* pParentItem;
  KShieldDirectory* pItem = FindFrom(m_pRoot, pwszFullPath, &pParentItem);
  if (pItem != NULL && pItem->m_pDirectoryInfo != NULL)
  {
    if (pItem->m_pDirectoryInfo->m_pUserContext != NULL)
    {
      if (ppUserContext != NULL)
        *ppUserContext = pItem->m_pDirectoryInfo->m_pUserContext;
    }
    else
    {
      pItem = NULL;
    }
    
    if (ppParentUserContext != NULL)
    {
      if (pParentItem != NULL && pParentItem->m_pDirectoryInfo != NULL)
        *ppParentUserContext = pParentItem->m_pDirectoryInfo->m_pUserContext;
    }
  }

  return pItem;
}

KShieldDirectory* KShieldDirectoryTree::FindMatch(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext)
{
  KLocker locker(&m_KSynchroObject);

  KShieldDirectory* pParentItem;
  KShieldDirectory* pItem = FindMatchFrom(m_pRoot, pwszFullPath, &pParentItem);
  if (pItem != NULL && pItem->m_pDirectoryInfo != NULL)
  {
    if (pItem->m_pDirectoryInfo->m_pUserContext != NULL)
    {
      if (ppUserContext != NULL)
        *ppUserContext = pItem->m_pDirectoryInfo->m_pUserContext;
    }
    else
    {
      pItem = NULL;
    }

    if (ppParentUserContext != NULL)
    {
      if (pParentItem != NULL && pParentItem->m_pDirectoryInfo != NULL)
        *ppParentUserContext = pParentItem->m_pDirectoryInfo->m_pUserContext;
    }
  }

  return pItem;
}

KShieldDirectory* KShieldDirectoryTree::FindMatchRest(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext)
{
  KLocker locker(&m_KSynchroObject);

  KShieldDirectory* pParentItem;
  KShieldDirectory* pItem = FindMatchRestFrom(m_pRoot, pwszFullPath, &pParentItem);
  if (pItem != NULL && pItem->m_pDirectoryInfo != NULL)
  {
    if (pItem->m_pDirectoryInfo->m_pUserContext != NULL)
    {
      if (ppUserContext != NULL)
        *ppUserContext = pItem->m_pDirectoryInfo->m_pUserContext;
    }
    else
    {
      pItem = NULL;
    }

    if (ppParentUserContext != NULL)
    {
      if (pParentItem != NULL && pParentItem->m_pDirectoryInfo != NULL)
        *ppParentUserContext = pParentItem->m_pDirectoryInfo->m_pUserContext;
    }
  }

  return pItem;
}

KShieldDirectory* KShieldDirectoryTree::FindFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir)
{
  KLocker locker(&m_KSynchroObject);

  if (pRoot == NULL)
    pRoot = m_pRoot;

  if (pRoot == NULL || pwszFullPath == NULL)
    return NULL;

  int nPathLen = wcslen(pwszFullPath);
  PWSTR pwszFullPathSave = new WCHAR[nPathLen + sizeof(WCHAR)];
  if (pwszFullPathSave == NULL)
    return NULL;
  wcscpy(pwszFullPathSave, pwszFullPath);

  KShieldDirectory* pItem = pRoot;
  PWSTR pwszLast;

  PWSTR pwszToken = _wcstok(pwszFullPathSave, L"\\/", &pwszLast);
  while (pwszToken != NULL)
  {
    pItem = pItem->Find(pwszToken);
    if (pItem == NULL)
      break;
    pwszToken = _wcstok(NULL, L"\\/", &pwszLast);
  }

  if (ppParentDir != NULL)
  {
    *ppParentDir = NULL;
    if (pItem != NULL)
      *ppParentDir = pItem->m_pParentDirectory;
  }

  delete[] pwszFullPathSave;

  return pItem;
}

KShieldDirectory* KShieldDirectoryTree::FindMatchFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir)
{
  KLocker locker(&m_KSynchroObject);

  if (pRoot == NULL)
    pRoot = m_pRoot;

  if (pRoot == NULL || pwszFullPath == NULL)
    return NULL;

  int nPathLen = wcslen(pwszFullPath);
  PWSTR pwszFullPathSave = new WCHAR[nPathLen + sizeof(WCHAR)];
  if (pwszFullPathSave == NULL)
    return NULL;
  wcscpy(pwszFullPathSave, pwszFullPath);

  KShieldDirectory* pItem = pRoot;
  PWSTR pwszLast;

  PWSTR pwszToken = _wcstok(pwszFullPathSave, L"\\/", &pwszLast);
  while (pwszToken != NULL)
  {
    pItem = pItem->FindMatch(pwszToken);
    if (pItem == NULL)
      break;
    pwszToken = _wcstok(NULL, L"\\/", &pwszLast);
  }

  if (ppParentDir != NULL)
  {
    *ppParentDir = NULL;
    if (pItem != NULL)
      *ppParentDir = pItem->m_pParentDirectory;
  }

  delete[] pwszFullPathSave;

  return pItem;
}

KShieldDirectory* KShieldDirectoryTree::FindMatchRestFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir)
{
  KLocker locker(&m_KSynchroObject);

  if (pRoot == NULL)
    pRoot = m_pRoot;

  if (pRoot == NULL || pwszFullPath == NULL)
    return NULL;

  KShieldDirectory* pItem = pRoot;
  KShieldDirectory* pItemPrev;
  PWSTR pwszToken = _skipdelimit(pwszFullPath, L"\\/");
  PWSTR pwszNextToken;
  WCHAR wSym;
  while (*pwszToken != 0)
  {
    pwszNextToken = _nextdelimit(pwszToken, L"\\/");
    wSym = *pwszNextToken;
    *pwszNextToken = 0;
    pItemPrev = pItem;
    pItem = pItem->FindMatch(pwszToken);
    *pwszNextToken = wSym;
    if (pItem == NULL || pItem->m_pDirectoryInfo == NULL)
    {
      if (pItem == NULL && *pwszNextToken != 0)
        pItem = pItemPrev->FindMatch(pwszToken);
      break;
    }
    if (pItem->m_pDirectoryInfo->m_bCompareRest == TRUE && pItem->IsEmpty() == TRUE)
    {
      if (MatchW(pwszToken, pItem->m_pDirectoryInfo->m_pwszName) == TRUE)
        break;
    }

    pwszToken = _skipdelimit(pwszNextToken, L"\\/");
  }

  if (ppParentDir != NULL)
  {
    *ppParentDir = NULL;
    if (pItem != NULL)
      *ppParentDir = pItem->m_pParentDirectory;
  }

  return pItem;
}