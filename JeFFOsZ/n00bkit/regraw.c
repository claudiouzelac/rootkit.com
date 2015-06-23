#include <windows.h>
#include "regraw.h"

static __inline void CleanDataRecord (PDATA_RECORD dr)
{
  DWORD size;

  size = REALSIZE (dr->RecordSize);
  memset (dr, 0, size);
  /* Replace size with positive to mark free. */
  dr->RecordSize = size;
}

static __inline void CleanVL (PVALUE_LIST vl)
{
  DWORD size;

  size = REALSIZE (vl->ListSize);
  memset (vl, 0, size);
  /* Replace size with positive to mark free. */
  vl->ListSize = size;
}

static __inline void CleanLI (PLI_RECORD li)
{
  DWORD size;

  size = REALSIZE (li->RecordSize);
  memset (li, 0, size);
  /* Replace size with positive to mark free. */
  li->RecordSize = size;
}

static __inline void CleanLF (PLF_RECORD lf)
{
  DWORD size;

  size = REALSIZE (lf->RecordSize);
  memset (lf, 0, size);
  /* Replace size with positive to mark free. */
  lf->RecordSize = size;
}

static __inline void CleanRI (PRI_RECORD ri)
{
  DWORD size;

  size = REALSIZE (ri->RecordSize);
  memset (ri, 0, size);
  /* Replace size with positive to mark free. */
  ri->RecordSize = size;
}

static void CleanVK (PVK_RECORD vk, PHBIN_BLOCK hbin)
{
  DWORD size;

  size = REALSIZE (vk->RecordSize);
  if (vk->Header != 'kv')
      return;
  
  if (!DATA_IN_VK_RECORD (vk) && VALID_HBIN_OFFSET (vk->DataRecord_offset))
  {
    CleanDataRecord (HBIN_REL (DATA_RECORD, vk->DataRecord_offset));
  }
  memset (vk, 0, size);
  /* Replace size with positive to mark free. */
  vk->RecordSize = size;
}

static void CleanSK (PSK_RECORD sk, PHBIN_BLOCK hbin)
{
  DWORD size;

  size = REALSIZE (sk->RecordSize);
  /* We must unlink it from the list. */
  if (VALID_HBIN_OFFSET (sk->NextRecord_offset))
  {
    PSK_RECORD next = HBIN_REL (SK_RECORD, sk->NextRecord_offset);
    next->LastRecord_offset = sk->LastRecord_offset;
  }
  if (VALID_HBIN_OFFSET (sk->LastRecord_offset))
  {
    PSK_RECORD last = HBIN_REL (SK_RECORD, sk->LastRecord_offset);
    last->NextRecord_offset = sk->NextRecord_offset;
  }
  memset (sk, 0, size);
  /* Replace size with positive to mark free. */
  sk->RecordSize = size;
}

static void CleanNK (PNK_RECORD nk, PHBIN_BLOCK hbin)
{
  DWORD size;

  size = REALSIZE (nk->RecordSize);
  if (nk->Header != 'kn')
      return;

  /* clean class name string if any. */
  if (nk->ClassNameLength && VALID_HBIN_OFFSET (nk->ClassName_offset))
  {
    CleanDataRecord (HBIN_REL (DATA_RECORD, nk->ClassName_offset));
  }
  /* dec sd ref cnt (if sd present) */
  if (VALID_HBIN_OFFSET (nk->SkRecord_offset))
  {
    PSK_RECORD sk = HBIN_REL (SK_RECORD, nk->SkRecord_offset);
    --sk->UsageCounter;
    /* If now unused, clean it */
    if (!sk->UsageCounter)
    {
      CleanSK (sk, hbin);
    }
  }

  /* values and subkey indices already cleared.  */
  memset (nk, 0, size);
  /* Replace size with positive to mark free. */
  nk->RecordSize = size;
  /* Woot! */
}

static void WalkHBinRootkey (PHBIN_BLOCK cur_hbin, PHBIN_BLOCK hbin, PCLEAN_NAME_TEST name_test)
{
  PNK_RECORD nk;

  nk = &cur_hbin->RootNkRecord;
  WalkKeyTree (nk, hbin, name_test, FALSE);
}

/* Map a view of a registry file, pass the base pointer to this. */
void CleanRegistryFile (void *data, PCLEAN_NAME_TEST name_test)
{
  PREGF_BLOCK regf = (PREGF_BLOCK) data;
  PHBIN_BLOCK hbin = &regf->FirstHbinBlock;

  if (regf->Header != 'fger')
     return;
  
  WalkHBinRootkey (HBIN_REL (HBIN_BLOCK, 0), hbin, name_test);
  return;
}

static void WalkKeyTree (PNK_RECORD nk, PHBIN_BLOCK hbin, PCLEAN_NAME_TEST name_test, char do_clean)
{
  DWORD size = REALSIZE (nk->RecordSize);

  if (nk->Header != 'kn')
     return;
  
  /* If offset to value list is not valid we have no values to clear.  */
  if (do_clean && VALID_HBIN_OFFSET (nk->ValueList_offset))
  {
    DWORD n, size;
    PVALUE_LIST vl = HBIN_REL (VALUE_LIST, nk->ValueList_offset);
    size = REALSIZE(vl->ListSize);
    for (n = 0; n < nk->NumberOfValues; n++)
    {
      if (VALID_HBIN_OFFSET (vl->VkRecords_offset[n]))
      {
        PVK_RECORD vk = HBIN_REL (VK_RECORD, vl->VkRecords_offset[n]);
        /* clean vk and data record */
        CleanVK (vk, hbin);
      }
    }
    /* clean vl */
    CleanVL (vl);
  }

  if (VALID_HBIN_OFFSET (nk->IndexRecord_offset))
  {
    PINDEX_RECORD index = HBIN_REL (INDEX_RECORD, nk->IndexRecord_offset);
    RI_RECORD temp_ri;
    PRI_RECORD ri;
    char clean_ri = FALSE;
    DWORD rec;
    /* we must check the sig. if it is an ri we have to iterate
     across multiple li or lf records, otherwise we give ourselves
     a fake ri with one entry to the single lf/li for simplicity. */
    if (index->Header == 'ir')
    {
      ri = (PRI_RECORD)index; /* that's what we want! */
    }
    else
    {
      /* build a fake one! */
      temp_ri.Header = 'ir';
      temp_ri.RecordSize = 0;
      temp_ri.NumberOfRecords = 1;
      /* oops! */
      /*temp_ri.IndexEntrys[0] = index;*/
      /* not that easy! We must make it hbin-relative!*/
      temp_ri.IndexEntrys[0] = (PINDEX_RECORD) HBIN_OFFS (index);
      /* sanity check */
      if ((index->Header != 'il') && (index->Header != 'fl'))
         temp_ri.NumberOfRecords = 0;
      
      ri = &temp_ri;
    }

    /* What are we waiting for? Let's go! */
    for (rec = 0; rec < ri->NumberOfRecords; rec++)
    {
      DWORD n, size;
      char collapse_ri = FALSE;
      PINDEX_RECORD index;

      /* check for safety.*/
      if (!VALID_HBIN_OFFSET (ri->IndexEntrys[rec]))
          continue;
     
      index = HBIN_REL (INDEX_RECORD, ri->IndexEntrys[rec]);
      if (index->Header == 'il')
      {
        PLI_RECORD li = HBIN_REL (LI_RECORD, ri->LiEntrys[rec]);
        char clean_li = FALSE;
        size = REALSIZE(li->RecordSize);

        for (n = 0; n < li->NumberOfKeys; n++)
        {
          char clean_this;
          if (VALID_HBIN_OFFSET (li->LiEntrys[n]))
          {
            PNK_RECORD subk = HBIN_REL (NK_RECORD, li->LiEntrys[n]);
            /* Do we want to destroy this key? */
            clean_this = do_clean || !(*name_test)(&subk->Name[0], subk->NameLength);
            /* now check/clean sub keys. */
            WalkKeyTree (subk, hbin, name_test, clean_this);
            if (clean_this && (clean_this != do_clean))
            {
              /* ok, we must compact the index. Is it going to be
               easier to do each match or save all for end? May as
               well do it all in one go as we unlikely to hide more 
               than one subkey of any given key most of the time.

               we want to shuffle down all subsequent entries. if the
               list has only 1 entry we delete it altogether. finally 
               we decrement the num subkeys in the master.

               and we might need to collapse the ri list as well. */
              nk->NumberOfSubKeys--;
              li->NumberOfKeys--;
              if (!li->NumberOfKeys)
              {
                /* remove li from ri and clean it. adjust outer loop. or
                 simpler, just clean li and signal outer (ri) loop to remove it.*/
                clean_li = TRUE;
                collapse_ri = TRUE;
              }
              else
              {
                if (n < li->NumberOfKeys)
                  RtlMoveMemory (&li->LiEntrys[n], &li->LiEntrys[n + 1],
                    (li->NumberOfKeys - n) * sizeof (PNK_RECORD));
                /* reset inner loop */
                --n;
              }
            };  /* if (we delete subkey n but not entire parent key) */
          };  /* if (entry n is valid) */
        };  /* end of inner (for n = num keys) loop */
        if (clean_li || do_clean)
        {
          /* clean il */
          CleanLI (li);
        }
      }
      else if (index->Header == 'fl')
      {
        PLF_RECORD lf = HBIN_REL (LF_RECORD, ri->LfEntrys[rec]);
        char clean_lf = FALSE;
        size = REALSIZE(lf->RecordSize);

        for (n = 0; n < lf->NumberOfKeys; n++)
        {
          char clean_this;
          if (VALID_HBIN_OFFSET (lf->LfEntrys[n].NkRecord_offset))
          {
            PNK_RECORD subk = HBIN_REL (NK_RECORD, lf->LfEntrys[n].NkRecord_offset);
            /* Do we want to destroy this key? */
            clean_this = do_clean || !(*name_test)(&subk->Name[0], subk->NameLength);
            /* now check/clean sub keys. */
            WalkKeyTree (subk, hbin, name_test, clean_this);
            if (clean_this && (clean_this != do_clean))
            {
              /* ok, we must compact the index (indexes). */
              nk->NumberOfSubKeys--;
              lf->NumberOfKeys--;
              if (!lf->NumberOfKeys)
              {
                /* clean li and signal outer (ri) loop to remove it. */
                clean_lf = TRUE;
                collapse_ri = TRUE;
                /* we are about to exit inner loop anyway. */
              }
              else
              {
                if (n < lf->NumberOfKeys)
                  RtlMoveMemory (&lf->LfEntrys[n], &lf->LfEntrys[n + 1],
                    (lf->NumberOfKeys - n) * sizeof (LF_ENTRY));
                /* reset inner loop */
                --n;
              }
            };  /* if (we delete subkey n but not entire parent key) */
          };  /* if (entry n is valid) */
        };  /* end of inner (for n = num keys) loop */
        if (clean_lf || do_clean)
        {
          /* clean fl */
          CleanLF (lf);
        }
      }
     
      /* ok: do we collapse the ri becuase the current entry (#rec)
       has no subkey entries any more after we deleted them? */
      if (collapse_ri)
      {
        --ri->NumberOfRecords;
        if (!ri->NumberOfRecords)
        {
          clean_ri = TRUE;
        }
        else
        {
          /* this won't ever happen for fake temp ri with one entry. */
          if (rec < ri->NumberOfRecords)
            RtlMoveMemory (&ri->IndexEntrys[rec], &ri->IndexEntrys[rec + 1],
              (ri->NumberOfRecords - rec) * sizeof (PINDEX_RECORD));
          /* reset outer loop */
          --rec;
        }
      };  /* end of (if we needed to remove an entire index from the ri */
    };  /* end of (for all records in ri) */
    if (clean_ri || do_clean)
    {
      /* clean ri if not temp fake */
      if (ri->RecordSize)
        CleanRI (ri);
      /* if we have removed entire ri/temp fake, remove ptr to it.
       don't bother if about to clean entire nk record anyway. */
      if (clean_ri && !do_clean)
      {
        /* we have cleaned all subkeys but not parent itself */
        nk->IndexRecord_offset = 0;
      }
    }
  }
  if (do_clean)
  {
    /* clean nk itself */
    CleanNK (nk, hbin);
  }
  /* Woot! */
}