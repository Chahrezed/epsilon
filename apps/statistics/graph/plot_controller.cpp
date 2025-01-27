#include "plot_controller.h"
#include <poincare/print.h>
#include <poincare/preferences.h>
#include "../data/store_controller.h"

namespace Statistics {

PlotController::PlotController(Escher::Responder * parentResponder,
                               Escher::ButtonRowController * header,
                               Escher::TabViewController * tabController,
                               Escher::StackViewController * stackViewController,
                               Escher::ViewController * typeViewController,
                               Store * store) :
    DataViewController(parentResponder, tabController, header, stackViewController, typeViewController, store),
    m_cursor(FLT_MAX),
    m_view(&m_curveView, &m_graphRange, &m_bannerView),
    // No bannerView given to the curve view because the display is handled here
    m_curveView(&m_graphRange, &m_cursor, nullptr, this)  {
}

void PlotController::moveCursorToSelectedIndex() {
  double x = valueAtIndex(m_selectedSeries, m_selectedIndex);
  double y = resultAtIndex(m_selectedSeries, m_selectedIndex);
  m_cursor.moveTo(x, x, y);
  m_curveView.reload();
}

void PlotController::viewWillAppearBeforeReload() {
  computeRanges(m_bannerView.bounds().height());
  // Sanitize m_selectedBarIndex and cursor's position
  m_selectedIndex = SanitizeIndex(m_selectedIndex, totalValues(m_selectedSeries));
  moveCursorToSelectedIndex();
}

bool PlotController::moveSelectionVertically(int direction) {
  int previousSeries = m_selectedSeries;
  bool result = DataViewController::moveSelectionVertically(direction);
  if (result && previousSeries != m_selectedSeries) {
    // Cursor has been moved into another curve, cursor must be switched
    moveCursorToSelectedIndex();
    // Reload to draw selected curve on top
    m_curveView.reload(false, true);
  }
  return result;
}

bool PlotController::reloadBannerView() {
  if (m_selectedSeries < 0) {
    return false;
  }
  KDCoordinate previousHeight = m_bannerView.minimalSizeForOptimalDisplay().height();

  int precision = Poincare::Preferences::sharedPreferences()->numberOfSignificantDigits();
  Poincare::Preferences::PrintFloatMode displayMode = Poincare::Preferences::sharedPreferences()->displayMode();
  constexpr static int k_bufferSize = 1 + Ion::Display::Width / KDFont::GlyphWidth(KDFont::Size::Small);
  char buffer[k_bufferSize] = "";

  // Display series name
  StoreController::FillSeriesName(m_selectedSeries, buffer, false);
  m_bannerView.seriesName()->setText(buffer);

  // Display selected value
  Poincare::Print::CustomPrintf(
    buffer,
    k_bufferSize,
    "%s%s%*.*ed",
    I18n::translate(I18n::Message::StatisticsValue),
    I18n::translate(I18n::Message::ColonConvention),
    m_cursor.x(), displayMode, precision);
  m_bannerView.value()->setText(buffer);

  // Display result value
  Poincare::Print::CustomPrintf(
    buffer,
    k_bufferSize,
    resultMessageTemplate(),
    I18n::translate(resultMessage()),
    I18n::translate(I18n::Message::ColonConvention),
    m_cursor.y(), displayMode, precision);
  m_bannerView.result()->setText(buffer);

  m_bannerView.reload();
  KDCoordinate newHeight = m_bannerView.minimalSizeForOptimalDisplay().height();
  if (previousHeight != newHeight) {
    computeRanges(newHeight);
    return true;
  }
  return false;
}

void PlotController::computeRanges(KDCoordinate bannerHeight) {
  float yMin, yMax, xMin, xMax;
  computeYBounds(&yMin, &yMax);
  computeXBounds(&xMin, &xMax);
  m_graphRange.calibrate(m_curveView.bounds().width(), m_curveView.bounds().height(), horizontalMargin(), bottomMargin() + bannerHeight, topMargin(), xMin, xMax, yMin, yMax);
}

void PlotController::computeXBounds(float * xMin, float *xMax) const {
  *xMin = m_store->minValueForAllSeries(handleNullFrequencies(), validSerieMethod());
  *xMax = m_store->maxValueForAllSeries(handleNullFrequencies(), validSerieMethod());
}

}
