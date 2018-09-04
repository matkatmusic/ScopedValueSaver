/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "ScopedValueSaver.h"

//==============================================================================
template<>
struct VariantConverter<Colour>
{
    static Colour fromVar (const var& v)
    {
        return Colour::fromString (v.toString());
    }
    
    static var toVar (const Colour& c)
    {
        return c.toString();
    }
};
//==============================================================================
struct Widget;
struct ColourSelectorWidget : public ColourSelector, public ChangeListener
{
    ColourSelectorWidget(Widget& widget);
    ~ColourSelectorWidget();
    void changeListenerCallback(ChangeBroadcaster* source) override;
private:
    ScopedValueSaver<Colour> csColour;
};
//==============================================================================
struct Widget : public Component
{
    Widget();
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& e) override;
    ScopedValueSaver<Colour> widgetColor;
};
//==============================================================================
class MainContentComponent   : public Component, public Button::Listener
{
public:
    MainContentComponent();
    ~MainContentComponent();
    
    void paint (Graphics&) override;
    void resized() override;
    
    void buttonClicked(Button* b) override;
private:
    Widget widget;
    TextButton showCSButton{"Show Color Selector"};
    ScopedPointer<ColourSelectorWidget> csWidget;
    bool showWidget{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
