/*
  ==============================================================================

    ScopedValueSaver.h
    Created: 4 Sep 2018 3:48:44pm
    Author:  Charles Schiermeyer

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#ifndef SCOPEDVALUESAVER_H_INCLUDED
#define SCOPEDVALUESAVER_H_INCLUDED

struct PropertyManager
{
    struct Property
    {
        Property() {}
        virtual ~Property() {}
        
        virtual void resetToDefault() = 0;
    };
    
    PropertyManager()
    {
        jassert( String(ProjectInfo::projectName).isNotEmpty() );
        jassert( String(ProjectInfo::companyName).isNotEmpty() );
        
        PropertiesFile::Options options;
        options.applicationName = ProjectInfo::projectName;
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = String(ProjectInfo::companyName) + File::separatorString + String(ProjectInfo::projectName);
        options.storageFormat = PropertiesFile::storeAsXML;
        
        appProperties.setStorageParameters(options);
        auto* settings = appProperties.getUserSettings();
        if( !settings->getFile().existsAsFile() )
        {
            settings->getFile().create();
        }
        settings->getFile().revealToUser();
    }
    
    ~PropertyManager()
    {
        appProperties.saveIfNeeded();
        DBG( "properties file path: " << appProperties.getUserSettings()->getFile().getFullPathName() );
    }
    
    ApplicationProperties& getProperties() { return appProperties; }
    
    void dump(StringRef prefix="settings: ")
    {
        DBG( prefix );
        DBG( appProperties.getUserSettings()->getFile().loadFileAsString() );
    }
    
    /**
     resets all registered properties to their default value.
     */
    void resetAllToDefault()
    {
        ScopedLock sl(propertyLock);
        for( int i = properties.size(); --i >= 0; )
        {
            properties.getUnchecked(i)->resetToDefault();
        }
    }
    
    void addProperty(Property* p)
    {
        ScopedLock sl(propertyLock);
        properties.addIfNotAlreadyThere(p);
    }
    
    void removeProperty(Property* p)
    {
        ScopedLock sl(propertyLock);
        properties.removeFirstMatchingValue(p);
    }
private:
    ApplicationProperties appProperties;
    Array<Property*> properties;
    CriticalSection propertyLock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyManager)
};

//==============================================================================
/**
 This class is a wrapper around a generic Type, and adds the following abilities:
 trigger a callback when the generic's value changes
 write the changed value to a properties file automatically every time the value changes.
 restore a value from the properties file the first time a ScopedValueSaver is created with
 a keyName that exists in the properties file
 */
template<typename Type>
struct ScopedValueSaver : public Value::Listener, public PropertyManager::Property
{
    /**
     creates a ScopedValueSaver with a specific name. if no other arguments are provided,
     the default value is set to an instance of Type(), via Type's default constructor
     
     @param name the name of the property to set in the ApplicationProperties file
     @param initialValue the initial value to use.  if omitted, its initialized to Type(). this value is used to populate the defaultValue and actualValue members
     @param changeFunc a lambda to call when the valueChanged callback is called.  This lets you express specific work, if any, that should happen when the underlying value object changes.
     */
    ScopedValueSaver(StringRef name,
                     const Type& initialValue=Type(),
                     std::function<void(Value&)> changeFunc = nullptr
                     ) :
    changeCallback(std::move(changeFunc)),
    keyName(name),
    defaultValue(initialValue),
    actualValue(initialValue)
    {
        setup();
        restore(initialValue);
    }
    
    /**
     creates a ScopedValueSaver from an existing juce::Value.
     
     @param valueToFollow an Existing juce::Value object
     @param name the name of the property to set in the ApplicationProperties file
     @param changeFunc a lambda to call when the valueChanged callback is called.  This lets you express specific work, if any, that should happen when the underlying value object changes.
     */
    ScopedValueSaver(const Value& valueToFollow,
                     StringRef name,
                     std::function<void(Value&)> changeFunc) :
    changeCallback(std::move(changeFunc)),
    keyName(name)
    {
        setup();
        value.referTo(valueToFollow);
        updateActualValue();
        updatePropertiesFile(); //create an entry in Properties as soon as we exist
    }
    
    ///copy constructor
    ScopedValueSaver (const ScopedValueSaver& other)
    {
        setup();
        value = other.value.getValue();
        updateActualValue();
        changeCallback = other.changeCallback;
        /*
         normally, the assignment above would trigger the valueChanged() callback
         but sometimes it doesn't, like if you're using this class before the
         MessageManager's runLoop is running.
         so, we'll tell the propertiesFile to update manually.
         when the valueChanged callback happens on the message thread, it'll update the properties file again.
         */
        updatePropertiesFile();
    }
    
    ///copy assignment operator
    ScopedValueSaver& operator= (const ScopedValueSaver& other) noexcept
    {
        setup();
        value = other.value.getValue();
        updateActualValue();
        changeCallback = other.changeCallback;
        updatePropertiesFile();
        return *this;
    }
    
    /*
     copy assigment operator
     
     this is called when you have code like this:
     ScopedValueSaver<float> svs("floatVal", 2.f);
     svs = 3.f;
     */
    template<typename OtherType>
    ScopedValueSaver<Type>& operator=( const OtherType& other)
    {
        //value.addListener(this); //value already had the listener added!
        value = VariantConverter<OtherType>::toVar(other);
        updateActualValue();
        
        updatePropertiesFile();
        return *this;
    }
    
    ~ScopedValueSaver()
    {
        value.removeListener(this);
        updatePropertiesFile();
    }
    
    void valueChanged(Value& changedVal) override
    {
        if( changedVal == value )
        {
            DBG( "value changed" );
            updateActualValue();
            updatePropertiesFile();
            //            props->dump("post-update");
            if( changeCallback )
            {
                changeCallback(value);
            }
        }
    }
    
    bool operator== (const ScopedValueSaver& other) const noexcept { return value == other.value; }
    bool operator!= (const ScopedValueSaver& other) const noexcept { return !(value == other.value); }
    
    /**
     Allows this object to behave like a juce::var.
     
     You might need to implement a VariantConverter<Type>::fromVar/toVar if one doesn't exist yet.
     
     this is called by static_cast<var>( ScopedValueSaver<Type>() );
     
     @return a juce::var version of Type.
     */
    operator var() const noexcept { return value.getValue(); }
    
    /**
     Allows this object to behave like a juce::Value
     
     this is called by static_cast<Value>( ScopedValueSaver<Type>() );
     or T::foo(const Value& v) e.g. T t; t.foo( scopedValueState );
     
     @return a juce::Value version of Type
     */
    operator Value() const noexcept { return value; }
    
    /**
     Allows this object to behave like Type
     
     VariantConverter<>::fromVar returns Type, so you'll need to implement a VariantConverter for your particular type.
     
     @return a Type
     */
    operator Type() const noexcept
    {
        return VariantConverter<Type>::fromVar(value.getValue());
    }
    
    ///changes the callback that will be executed when the internal juce::Value is modified
    void setChangeCallback(std::function<void(Value&)> callback)
    {
        changeCallback = std::move(callback);
    }
    
    /**
     changes the keyName used to store this property.
     
     This will erase the old keyname and value from the settings file, and replace it with the new one.
     */
    void setKeyName(StringRef name)
    {
        props->getProperties().getUserSettings()->removeValue(keyName);
        keyName = name;
        updatePropertiesFile();
    }
    
    /**
     resets the internal juce::Value and Type actualValue objects to the default value
     */
    void resetToDefault() override
    {
        value = VariantConverter<Type>::toVar( defaultValue );
        actualValue = defaultValue;
        updatePropertiesFile();
    }
    
    /**
     allows you to access member functions and variables on your Type.
     Be advised that this does not automatically save if you modify a member variable.
     for this reason, save() is provided as a way to manually save your value to disk
     after modifying a member of it.
     */
    Type& getActualValue() { return actualValue; }
    
    /**
     updates the internal juce::Value object to match the value of actualValue.
     this is typically called after using getActualValue() to modify members of Type.
     it also updates the settings file on disk.
     */
    void save()
    {
        value.removeListener(this);
        value = VariantConverter<Type>::toVar( actualValue );
        value.addListener(this);
        updatePropertiesFile();
    }
private:
    void setup()
    {
        props->addProperty(this);
        value.addListener(this);
    }
    
    void updateActualValue()
    {
        actualValue = VariantConverter<Type>::fromVar(value.getValue());
    }
    
    void updatePropertiesFile()
    {
        if( keyName.isNotEmpty() )
        {
            DBG( "updating properties with changed value for: " << keyName );
            props->getProperties().getUserSettings()->setValue(keyName,
                                                               value);
            props->getProperties().saveIfNeeded();
        }
    }
    
    void restore(const Type& initialValue)
    {
        /*
         the properties are stored as Strings
         the initialValue is not a string, so it must be converted to a string representation
         of the Type.
         then, initialValue can be used as a default value when pulling a value from the properties file
         the returned value will be a string, so this must then be converted to Type before it can be converted to a var that the VariantConverter<Type> can understand
         the best way to do that is:
         <String>::toVar() -> <Type>::fromVar() -> <Type>::toVar()
         
         the reason we can't do String -> Type is because we don't know if Type will have a ctor
         that accepts a string.
         but we do know that there will be a VariantConverter<Type>::fromVar/toVar
         and that there is a VariantConverter<String>::toVar(str)
         */
        String defaultValStr = VariantConverter<Type>::toVar(initialValue).toString();
        
        String propStrVal = props->getProperties().getUserSettings()->getValue(keyName,
                                                                               defaultValStr);
        var tempVar = VariantConverter<String>::toVar(propStrVal);
        
        Type tempType = VariantConverter<Type>::fromVar(tempVar);
        
        var properVar = VariantConverter<Type>::toVar(tempType);
        
        value = properVar;
        updateActualValue();
        updatePropertiesFile();
    }
    
    ///the internal listenable value object
    juce::Value value;
    
    ///the callback to fire when the value changes
    std::function<void(juce::Value&)> changeCallback;
    
    /**
     the SharedResourcePointer is like a combination of a Singleton and a std::shared_ptr.
     the first time a ScopedValueSaver is created, an instance of PropertyManager is created,
     which automatically creates the proper settings file on disk.
     */
    juce::SharedResourcePointer<PropertyManager> props;
    
    ///the name to use when writing/reading this value to/from disk.
    juce::StringRef keyName;
    
    /**
     the default value.  this is initialized when you first create a ScopedValueSaver using the initialValue constructor
     if you use the other constructor, this is initialized via the = Type().  your Type is expected to have a default constructor
     */
    Type defaultValue = Type();
    
    
    Type actualValue;
    
    JUCE_LEAK_DETECTOR(ScopedValueSaver)
};

#endif  // SCOPEDVALUESAVER_H_INCLUDED

