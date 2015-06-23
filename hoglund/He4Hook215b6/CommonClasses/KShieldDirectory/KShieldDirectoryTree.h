#ifndef __KSHIELDDIRECTORY_TREE_H
 #define __KSHIELDDIRECTORY_TREE_H

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
//#include "../KNativeSynchroObject/KNativeSynchroObject.h"
#include "KShieldDirectory.h"

class KShieldDirectoryTree;
//*******************************************************************//

class KShieldDirectoryTree
{
  public:
   explicit
   KShieldDirectoryTree();
   virtual ~KShieldDirectoryTree();

   // во всех этих ф-ях pUserContext должен быть отличен от NULL,
   // даже если вам он не нужен, это необходимо чтобы отличить каталоги,
   // которые вы добавили от промежуточных.
   // пример:
   //  добавляем "\\Device\\Harddisk0\\Partition0\\i386"
   //  в дереве будут созданы следующие узлы
   //   Device
   //   Harddisk0
   //   Partition0
   //   i386
   //  так вот во всех узлах кроме i386, pUserContext будет равен 0,
   //  а для i386 - тому что вы передали.
   BOOLEAN                Add(PWSTR pwszFullPath, PVOID pUserContext);
   BOOLEAN                Remove(PWSTR pwszFullPath, PVOID* ppUserContext);
   KShieldDirectory*      Find(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext = NULL);
   KShieldDirectory*      FindMatch(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext = NULL);
   KShieldDirectory*      FindMatchRest(PWSTR pwszFullPath, PVOID* ppUserContext, PVOID* ppParentUserContext = NULL);

  //protected:
   // эти ф-ии снаружи пользовать очень аккуратно, потому как если вы
   // получили в первом вызове KShieldDirectory*, то второй вызов с указанием
   // в качестве pRoot этого указателя может привести к падению, если кто-то
   // успел удалить этот узел. для всех этих ф-й справедливо: 
   // if (pRoot == NULL) pRoot = m_pRoot;
   KShieldDirectory*      FindFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir = NULL);
   KShieldDirectory*      FindMatchFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir = NULL);
   KShieldDirectory*      FindMatchRestFrom(KShieldDirectory* pRoot, PWSTR pwszFullPath, KShieldDirectory** ppParentDir = NULL);

  private:
   KShieldDirectoryTree(const KShieldDirectoryTree&);
   KShieldDirectoryTree& operator=(const KShieldDirectoryTree& right);

  protected:
   KShieldDirectory*      m_pRoot;

   KSpinSynchroObject     m_KSynchroObject;
   //KNativeSynchroObject   m_KSynchroObject;
};


#endif //__KSHIELDDIRECTORY_TREE_H