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
struct ValuePlus : public Value::Listener
{
    ValuePlus(StringRef n) : name(n) { value.addListener(this); }
    ValuePlus(const Value& valueToFollow, StringRef n) : name(n)
    {
        value.referTo(valueToFollow);
        value.addListener(this);
    }
    ~ValuePlus() { value.removeListener(this); }
    
    template<typename OtherType>
    ValuePlus& operator=(const OtherType& other)
    {
        value = VariantConverter<OtherType>::toVar(other);
        return *this;
    }
    
    Value value;
    StringRef name;
    void valueChanged(Value& v) override { DBG( name << " changed. new value: " << value.toString()); }
    operator Value() const noexcept { return value; }
    const Value& getValue() { return value; }
};
//==============================================================================
class MainContentComponent   :
public Component,
public Button::Listener,
public Timer
{
public:
    MainContentComponent();
    ~MainContentComponent();
    
    void paint (Graphics&) override;
    void resized() override;
    
    void buttonClicked(Button* b) override;
    void timerCallback() override;
private:
    Widget widget;
    TextButton showCSButton{"Show Color Selector"};
    ScopedPointer<ColourSelectorWidget> csWidget;
    bool showWidget{false};
    
    ValuePlus a;
    ValuePlus aFollower;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
