// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/performance_manager/freezing/freezer.h"

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/task_traits.h"
#include "components/performance_manager/public/graph/page_node.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"

namespace performance_manager {
namespace {

// Try to freeze a page on the UI thread.
void MaybeFreezePageOnUIThread(base::WeakPtr<content::WebContents> contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!contents) {
    return;
  }

  // A visible page should not be frozen.
  if (contents->GetVisibility() == content::Visibility::VISIBLE) {
    return;
  }

  contents->SetPageFrozen(true);
}

void UnfreezePageOnUIThread(base::WeakPtr<content::WebContents> contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!contents) {
    return;
  }

  // A visible page is automatically unfrozen.
  if (contents->GetVisibility() == content::Visibility::VISIBLE) {
    return;
  }

  contents->SetPageFrozen(false);
}

}  // namespace

void Freezer::MaybeFreezePageNode(const PageNode* page_node) {
  DCHECK(page_node);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&MaybeFreezePageOnUIThread, page_node->GetWebContents()));
}

void Freezer::UnfreezePageNode(const PageNode* page_node) {
  DCHECK(page_node);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&UnfreezePageOnUIThread, page_node->GetWebContents()));
}

}  // namespace performance_manager
