#ifndef IAITOWIDGET_H
#define IAITOWIDGET_H

#include "common/RefreshDeferrer.h"
#include "core/IaitoCommon.h"

#include <QDockWidget>

class MainWindow;

class IAITO_EXPORT IaitoDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    IAITO_DEPRECATED("Action will be ignored. Use IaitoDockWidget(MainWindow*) instead.")
    IaitoDockWidget(MainWindow *parent, QAction *action);

    explicit IaitoDockWidget(MainWindow *parent);
    ~IaitoDockWidget() override;
    bool eventFilter(QObject *object, QEvent *event) override;
    bool isVisibleToUser() { return isVisibleToUserCurrent; }

    /**
     * @brief Set whether the Widget should be deleted after it is closed.
     * This is especially important for extra widgets.
     */
    void setTransient(bool v) { isTransient = v; }

    /**
     * @brief Convenience method for creating and registering a RefreshDeferrer
     * without any parameters
     * @param refreshNowFunc lambda taking no parameters, called when a refresh
     * should occur
     */
    template<typename Func>
    RefreshDeferrer *createRefreshDeferrer(Func refreshNowFunc)
    {
        auto *deferrer = new RefreshDeferrer(nullptr, this);
        deferrer->registerFor(this);
        connect(
            deferrer,
            &RefreshDeferrer::refreshNow,
            this,
            [refreshNowFunc](const RefreshDeferrerParamsResult) { refreshNowFunc(); });
        return deferrer;
    }

    /**
     * @brief Convenience method for creating and registering a RefreshDeferrer
     * with a replacing Accumulator
     * @param replaceIfNull passed to the ReplacingRefreshDeferrerAccumulator
     * @param refreshNowFunc lambda taking a single parameter of type
     * ParamResult, called when a refresh should occur
     */
    template<class ParamResult, typename Func>
    RefreshDeferrer *createReplacingRefreshDeferrer(bool replaceIfNull, Func refreshNowFunc)
    {
        auto *deferrer = new RefreshDeferrer(
            new ReplacingRefreshDeferrerAccumulator<ParamResult>(replaceIfNull), this);
        deferrer->registerFor(this);
        connect(
            deferrer,
            &RefreshDeferrer::refreshNow,
            this,
            [refreshNowFunc](const RefreshDeferrerParamsResult paramsResult) {
                auto *result = static_cast<const ParamResult *>(paramsResult);
                refreshNowFunc(result);
            });
        return deferrer;
    }
    /**
     * @brief Serialize dock properties for saving as part of layout.
     *
     * Override this function for saving dock specific view properties. Use
     * in situations where it makes sense to have different properties for
     * multiple instances of widget. Don't use for options that are more
     * suitable as global settings and should be applied equally to all widgets
     * or all widgets of this kind.
     *
     * Keep synchrononized with deserializeViewProperties. When modifying add
     * project upgrade step in SettingsUpgrade.cpp if necessary.
     *
     * @return Dictionary of current dock properties.
     * @see IaitoDockWidget#deserializeViewProperties
     */
    virtual QVariantMap serializeViewProprties();
    /**
     * @brief Deserialization half of serialize view properties.
     *
     * When a property is not specified in property map dock should reset it
     * to default value instead of leaving it umodified. Empty map should reset
     * all properties controlled by
     * serializeViewProprties/deserializeViewProperties mechanism.
     *
     * @param properties to modify for current widget
     * @see IaitoDockWidget#serializeViewProprties
     */
    virtual void deserializeViewProperties(const QVariantMap &properties);
    /**
     * @brief Ignore visibility status.
     * Useful for temporary ignoring visibility changes while this information
     * is unreliable.
     * @param ignored - set to true for enabling ignoring mode
     */
    void ignoreVisibilityStatus(bool ignored);

    void raiseMemoryWidget();
signals:
    void becameVisibleToUser();
    void closed();

public slots:
    void toggleDockWidget(bool show);

protected:
    virtual QWidget *widgetToFocusOnRaise();

    void closeEvent(QCloseEvent *event) override;
    QString getDockNumber();

    MainWindow *mainWindow;

private:
    bool isTransient = false;

    bool isVisibleToUserCurrent = false;
    bool ignoreVisibility = false;
    void updateIsVisibleToUser();
};

#endif // IAITOWIDGET_H
