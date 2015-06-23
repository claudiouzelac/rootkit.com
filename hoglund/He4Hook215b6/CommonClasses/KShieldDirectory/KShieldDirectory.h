#ifndef __KSHIELDDIRECTORY_H
 #define __KSHIELDDIRECTORY_H

#ifndef __TEST_WIN32
 extern "C"
 {
  #include "ntddk.h"
 }
 
 #include "../Include/KNew.h"
 #include "../Include/KTypes.h"
 #include "../KStdLib/krnlstdlib.h"
#else
 #include <windows.h>
#endif //__TEST_WIN32

#include "../KSpinSynchroObject/KSpinSynchroObject.h"

class KDLinkedList;
class KBinaryTree;
class KShieldDirectory;
//*******************************************************************//
#define SDIR_SEARCH_BY_INFO      0
#define SDIR_SEARCH_BY_NAME      1

class KShieldDirectory
{
   friend class KShieldDirectoryTree;
  public:
   typedef struct tag_DIRECTORY_INFO
   {
     PWSTR        m_pwszName;
     int          m_nSizeName;    // without zero end (by bytes)
     BOOLEAN      m_bCompareRest; // if IsEmpty() == TRUE && into m_pwszName exist '?' or '*'
     PVOID        m_pUserContext; //
     ULONG        m_dwUserCount;
     ULONG        m_dwChildItemsCount;

     tag_DIRECTORY_INFO()
     {
       m_pwszName = NULL;   
       m_nSizeName = 0;  
       m_bCompareRest = FALSE;
       m_pUserContext = NULL;
       m_dwUserCount = 0;
       m_dwChildItemsCount = 0;
     };
     ~tag_DIRECTORY_INFO()
     {
       if (m_pwszName != NULL)
         delete m_pwszName;
       m_pwszName = NULL;
     };
   } DIRECTORY_INFO, *PDIRECTORY_INFO;

   explicit
   KShieldDirectory(KShieldDirectory* pParentDirectory);
   virtual ~KShieldDirectory();

   virtual BOOLEAN        Open(PWSTR pwszName, PVOID pUserContext = NULL);
   virtual void           Close();
   virtual KShieldDirectory* Add(PWSTR pwszName, PVOID pUserContext = NULL);
   virtual BOOLEAN           Remove(PWSTR pwszName);
   virtual KShieldDirectory* Find(PWSTR pwszName);
   virtual KShieldDirectory* FindMatch(PWSTR pwszName);
           BOOLEAN        IsEmpty();

  protected:
   virtual PDIRECTORY_INFO CreateDirectoryInfo(PWSTR pwszName, PVOID pUserContext = NULL);

  private:
   KShieldDirectory(const KShieldDirectory&);
   KShieldDirectory& operator=(const KShieldDirectory& right);

           BOOLEAN        IsMatchedName(PWSTR pwszName, WCHAR Symbol);

   static int             _CompareFullNames(PVOID* pData1, PVOID* pData2, ULONG dwParam);
   static int             CompareDirInfo(PDIRECTORY_INFO pDirInfo1, PDIRECTORY_INFO pDirInfo2);
   static int             CompareNameAndDirInfo(PDIRECTORY_INFO pDirInfo, PWSTR pwszName);

  public:
   PDIRECTORY_INFO        m_pDirectoryInfo;
   KShieldDirectory*      m_pParentDirectory;

  protected:
   KBinaryTree*           m_KBTreeFullNameItems;
   KDLinkedList*          m_pDListMatchNameItems;
   BOOLEAN                m_bOpen;

   KSpinSynchroObject     m_KSynchroObject;
   //KNativeSynchroObject   m_KSynchroObject;
};


#endif //__KSHIELDDIRECTORY_H