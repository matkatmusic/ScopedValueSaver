/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"


//==============================================================================
ColourSelectorWidget::ColourSelectorWidget(Widget& widget) :
/*
 csColour "follows" the value of widget.widgetColor.
 whenever widget.widgetColor's value changes, the lambda
 in the constructor of csColour will be called
 */
csColour( widget.widgetColor.operator Value(),
         "csColour",
         [this]( Value& v )
         {
             if( v == csColour )
             {
                 /*
                  csColour converts to Colour via ScopedValueSaver<>::operator Type()
                  */
                 this->setCurrentColour(csColour);
             }
         })
{
    addChangeListener (this);
}

ColourSelectorWidget::~ColourSelectorWidget()
{
    removeChangeListener(this);
}

void ColourSelectorWidget::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if( source == this )
    {
        csColour = getCurrentColour();
    }
}
//==============================================================================
Widget::Widget() : widgetColor("widgetColor",
                               Colours::red,
                               [this](Value&) {this->repaint();} )
{
    setPaintingIsUnclipped(true);
}

void Widget::paint(juce::Graphics &g)
{
    g.setColour(Colours::black);
    g.fillRect(getLocalBounds());
    
    /*
     widgetColor converts to Colour via ScopedValueSaver<>::operator Type()
     */
    g.setColour( widgetColor );
    g.fillRect( getLocalBounds().reduced(3) );
}

void Widget::mouseDown(const MouseEvent& e)
{
    Random random;
    uint8 r = random.nextInt({0,255});
    uint8 g = random.nextInt({0,255});
    uint8 b = random.nextInt({0,255});
    
    /*
     this calls
     template<typename OtherType>
     ScopedValueSaver<Type>& operator=( const OtherType& other)
     
     because ColourSelectorWidget.csColour is following widgetColor, it will also be updated when widgetColor is changed.
     the csColour lambda will cause the ColourSelector to change its color to widgetColor's value
     */
    widgetColor = Colour(r, g, b);
}
//==============================================================================
MainContentComponent::MainContentComponent()
{
    addAndMakeVisible(widget);
    addAndMakeVisible(showCSButton);
    showCSButton.addListener(this);
#if JUCE_IOS
    auto area = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
    setSize( area.getWidth(), area.getHeight() );
#else
    setSize (600, 400);
#endif
}

MainContentComponent::~MainContentComponent()
{
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll (Colours::white);
}

void MainContentComponent::buttonClicked(juce::Button *b)
{
    if( b == &showCSButton )
    {
        showWidget = !showWidget;
        if( showWidget )
        {
            csWidget = new ColourSelectorWidget(widget);
            addAndMakeVisible(csWidget);
            resized();
            showCSButton.setButtonText("Hide Color Selector");
        }
        else
        {
            csWidget = nullptr;
            showCSButton.setButtonText("Show Color Selector");
        }
    }
}

void MainContentComponent::resized()
{
    widget.setBounds(10, 10, 30, 30);
    showCSButton.changeWidthToFitText(20);
    showCSButton.setTopLeftPosition(10, 50);
    if( showWidget )
        csWidget->setBounds(200, 0, getWidth() - 200, getHeight());
}
