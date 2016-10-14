#ifndef GRAPH_TITLE_CELL_H
#define GRAPH_TITLE_CELL_H

#include <escher.h>
#include "../even_odd_cell.h"

namespace Graph {
class TitleCell : public EvenOddCell {
public:
  TitleCell();
  void reloadCell() override;
  void setText(const char * textContent);
  int numberOfSubviews() const override;
  View * subviewAtIndex(int index) override;
  void layoutSubviews() override;
  void drawRect(KDContext * ctx, KDRect rect) const override;

protected:
  PointerTextView m_pointerTextView;
};

}

#endif
