Setup consist of 2 scripts, 2 automations, and several entities configured manually.

## Scripts

Scripts are defined using UI and call mqtt service for tunrning on/off the irrigation (hence the names):

### Irrigation On

```yaml
alias: Irrigation On
mode: single
icon: mdi:water-pump
sequence:
  - service: mqtt.publish
    data_template:
      topic_template: hunter/X-CORE/zone/{{ states('sensor.irrigate_map_zone') }}
      payload_template: >
        { "action": "start", "time": {{ states('input_number.irrigate_timer_' ~
        states('sensor.irrigate_map_zone')) | int }} }
    enabled: true
```

### Irrigation Off

```yaml
alias: Irrigation Off
mode: single
icon: mdi:water-pump-off
sequence:
  - service: mqtt.publish
    data_template:
      topic_template: hunter/X-CORE/zone/{{ states('input_text.irrigation_active_zone') }}
      payload_template: |
        { "action": "stop" }
    enabled: true
```

## Automations

There are 2 separate automations, one is used to listen to specific MQTT topic, second turns off the irrigation on countdown reached zero. They are managed using UI.

### MQTT Listener for irrigation messages

```yaml
alias: mqtt_hook_Irrigation
description: "MQTT listener which receives response about triggered irrigation command. "
trigger:
  - platform: mqtt
    topic: hunter/X-CORE/result
condition: []
action:
  - service: input_text.set_value
    data_template:
      entity_id: input_text.zone_action_result
      value: "{{ trigger.payload_json.action }} ({{trigger.payload_json.result }})"
mode: single
```

### Reset switch on countdown finish

```yaml
alias: Irrigation - Reset switch when timer finishes
description: ""
trigger:
  - platform: event
    event_type: timer.finished
    event_data:
      entity_id: timer.irrigation_time_remaining
condition: []
action:
  - service: switch.turn_off
    data: {}
    target:
      entity_id: switch.irrigate_lawn
mode: single
```

## Entities

Following entities were set up manually in `homeassistant/configuration.yaml`.
4 zones with separate configurable time-selectors were created, count-down timer and switch to toggle irrigation on/off.

Whole script:

```yaml
##################################
# Irrigate zone selectbox       #
##################################
input_select:
  irrigate_zone:
    name: Irrigate zone
    options:
      - zone1
      - zone2
      - zone3
      - zone4

template:
  - sensor:
      - name: irrigate_map_zone
        state: >
          {% set mapper =
            { 'zone1':'1',
              'zone2':'2',
              'zone3':'3',
              'zone4':'4'} %}
          {% set state = states('input_select.irrigate_zone') %}
          {% set id = mapper[state] if state in mapper %}
          {{ id }}

input_text:
  irrigation_active_zone:
    initial: inactive
  zone_action_result:
    name: "Zone Action Result"

##################################
# Irrigation zone time selection   #
##################################
input_number:
  irrigate_timer_1:
    name: Timer zone1
    initial: 30
    min: 1
    max: 60
    step: 1
    unit_of_measurement: min
    icon: mdi:timer-cog

  irrigate_timer_2:
    name: Timer zone2
    initial: 30
    min: 1
    max: 60
    step: 1
    unit_of_measurement: min
    icon: mdi:timer-cog

  irrigate_timer_3:
    name: Timer zone3
    initial: 30
    min: 1
    max: 60
    step: 1
    unit_of_measurement: min
    icon: mdi:timer-cog

  irrigate_timer_4:
    name: Timer zone4
    initial: 30
    min: 1
    max: 60
    step: 1
    unit_of_measurement: min
    icon: mdi:timer-cog

################################################
# Selected Sprinkler switch script & timer #
################################################
timer:
  irrigation_time_remaining:
    name: Remaining Time
    duration: "00:00:00"

switch:
  - platform: template
    switches:
      irrigate_lawn:
        friendly_name: "Sprinkler On/Off"
        turn_on:
          - service: script.turn_on
            data:
              entity_id: script.irrigation_on
          - service: timer.start
            target:
              entity_id: timer.irrigation_time_remaining
            data:
              duration: "{{ states('input_number.irrigate_timer_' ~ states('sensor.irrigate_map_zone')) | int  * 60 }}"
          - service: input_text.set_value
            target:
              entity_id: input_text.irrigation_active_zone
            data:
              value: "{{ states('sensor.irrigate_map_zone') }}"
        turn_off:
          - service: script.turn_on
            data:
              entity_id: script.irrigation_off
          - service: timer.finish
            target:
              entity_id: timer.irrigation_time_remaining
          - service: input_text.set_value
            target:
              entity_id: input_text.irrigation_active_zone
            data:
              value: inactive
        icon_template: >-
          mdi:water-pump
```

## Irrigation control card (user interface)

Requirements:

- [HACS](https://hacs.xyz/) installed
- [mushrooom-ui](https://github.com/piitaya/lovelace-mushroom)
- [Button card](https://github.com/custom-cards/button-card)

Control is made of several cards divided into 3 rows:

1. Title created using `mushroom-title-card`
2. Zone selection of the left side with (conditionaly displayed) timer for currently selected zone on the right
3. Custom card which displays current state, debug message and toggles irrigation.
   - its look match [mushrooom-ui](https://github.com/piitaya/lovelace-mushroom) and was created using [Button card](https://github.com/custom-cards/button-card) plugin
   - all necessary info can be found [in this repo](https://github.com/marek-polak/lovelace-mushroom-button-cards).
   - there is no visual editor available for it, so it should be defined as:
     ```yaml
     type: custom:button-card
     template: mushroom_irrigation
     entity: switch.irrigate_lawn # switch defined in configuration.yaml
     variables:
       timer: timer.irrigation_time_remaining # timer from configuration.yaml
       active_zone: input_text.irrigation_active_zone # inputtext entity from configuration.yaml
       response: input_text.zone_action_result # inputtext entity from configuration.yaml
     ```

YAML config for the whole looks like this (english version is available in [irrigation-control.en.yaml](/docs/scripts/irrigation-control.en.yaml)):

```yaml
type: vertical-stack
cards:
  - type: custom:mushroom-title-card
    title: Závlaha
  - type: horizontal-stack
    cards:
      - type: custom:mushroom-select-card
        entity: input_select.irrigate_zone
        icon: mdi:sprinkler
        name: Zóna
      - type: conditional
        conditions:
          - entity: sensor.irrigate_map_zone
            state: "1"
        card:
          type: custom:mushroom-number-card
          entity: input_number.irrigate_timer_1
          name: Dĺžka závlahy
          primary_info: name
          tap_action:
            action: none
          hold_action:
            action: none
          double_tap_action:
            action: none
      - type: conditional
        conditions:
          - entity: sensor.irrigate_map_zone
            state: "2"
        card:
          type: custom:mushroom-number-card
          entity: input_number.irrigate_timer_2
          name: Dĺžka závlahy
          primary_info: name
          tap_action:
            action: none
          hold_action:
            action: none
          double_tap_action:
            action: none
      - type: conditional
        conditions:
          - entity: sensor.irrigate_map_zone
            state: "3"
        card:
          type: custom:mushroom-number-card
          entity: input_number.irrigate_timer_3
          name: Dĺžka závlahy
          primary_info: name
          tap_action:
            action: none
          hold_action:
            action: none
          double_tap_action:
            action: none
      - type: conditional
        conditions:
          - entity: sensor.irrigate_map_zone
            state: "4"
        card:
          type: custom:mushroom-number-card
          entity: input_number.irrigate_timer_4
          name: Dĺžka závlahy
          primary_info: name
          tap_action:
            action: none
          hold_action:
            action: none
          double_tap_action:
            action: none
  - type: custom:button-card
    template: mushroom_irrigation
    entity: switch.irrigate_lawn
    variables:
      timer: timer.irrigation_time_remaining
      active_zone: input_text.irrigation_active_zone
      response: input_text.zone_action_result
```

### button-card template

For the control to work, following button-card templates need to be defined.
Go to **raw configuration editor** for dashboard:

![dashboard ](/docs/images/ha/dashboard_options.png "HA - dashboard - raw configuration editor ")

and manually paste into file (english version is available in [custom-button-card-templates.en.yaml](/docs/scripts/custom-button-card-templates.en.yaml)):

```yaml
button_card_templates:
  mushroom_card:
    show_state: true
    hold_action:
      action: more-info
    custom_fields:
      notification: |
        [[[
          if (entity.state =="unavailable"){
            return `<ha-icon icon="mdi:exclamation" style="width: 12px; height: 12px; color: var(--primary-background-color);"></ha-icon>`
          }
        ]]]
    state:
      - value: unavailable
        id: unavail
        styles:
          custom_fields:
            notification:
              - border-radius: 50%
              - position: absolute
              - left: 24px
              - top: "-2px"
              - height: 16px
              - width: 16px
              - border: 2px solid var(--card-background-color)
              - font-size: 12px
              - line-height: 14px
              - background-color: |
                  [[[
                    return "rgba(var(--color-red),1)";
                  ]]]
    styles:
      card:
        - padding: 12px
        - "--mdc-ripple-press-opacity": 0
      grid:
        - grid-template-areas: '"i n" "i s"'
        - grid-template-columns: min-content 1fr
        - grid-template-rows: min-content min-content
      img_cell:
        - background-color: rgba(var(--rgb-primary-text-color), 0.05)
        - border-radius: var(--icon-border-radius)
        - border-radius: 50%
        - place-self: flex-start
        - width: 42px
        - height: 42px
        - margin-right: 12px
      icon:
        - color: var(--disabled-text-color)
        - width: 20px
        - height: 20px
      name:
        - justify-self: flex-start
        - align-self: center
        - font-size: 14px
        - font-weight: bold
        - margin-top: 4px
        - color: var(--primary-text-color)
      state:
        - justify-self: flex-start
        - align-self: center
        - font-weight: bolder
        - font-size: 12px
        - padding-top: 3px
        - color: rgb(114,114,114)
  mushroom_button:
    variables:
      color: var(--primary-text-color)
      background_color: rgba(var(--rgb-primary-text-color), 0.05)
    styles:
      card:
        - box-shadow: none
        - min-width: 42px
        - height: 42px
        - padding: 0
        - border-radius: 12px
        - border-width: 0
        - background-color: "[[[ return variables.background_color ]]]"
        - color: "[[[ return variables.color ]]]"
      name:
        - font-weight: bold
      icon:
        - width: 20px
        - color: "[[[ return variables.color ]]]"
  debug_text:
    show_name: true
    show_icon: false
    tap_action:
      action: none
    styles:
      card:
        - box-shadow: none
        - margin: 12px 0 0 0
        - padding: 0
        - border-radius: 0
        - border-width: 0
        - "--mdc-ripple-press-opacity": 0
      grid:
        - grid-template-areas: i n
        - grid-template-columns: min-content
        - grid-template-rows: min-content
  mushroom_irrigation:
    template: mushroom_card
    variables:
      timer: ""
      response: ""
      active_zone: ""
    name: |
      [[[
            if (entity.state =='off')
              return "Závlaha neaktívna";
            else
              return `Zavlažujem zónu ${states[variables.active_zone].state}`;
      ]]]
    state_display: >
      [[[ return `<ha-icon icon="mdi:message-text" style="width: 12px; height:
      12px; color: var(--primary-font-color);"></ha-icon>: ` + 
          states[variables.response].state;
      ]]]
    icon: |
      [[[
            if (entity.state =='off'){ return "mdi:water-pump-off";}
            return "mdi:water-pump";
      ]]]
    triggers_update: all
    styles:
      grid:
        - grid-template-areas: '"i n timer switch" "i s timer switch"'
        - grid-template-columns: min-content 1fr min-content min-content
        - grid-template-rows: min-content min-content
    state:
      - value: "on"
        styles:
          img_cell:
            - background-color: rgba(139,195,74,0.2)
          icon:
            - color: rgb(var(--rgb-accent-color))
    custom_fields:
      switch:
        card:
          type: custom:button-card
          template: mushroom_button
          entity: "[[[ return entity.entity_id ]]]"
          show_name: false
          icon: >-
            [[[ return (entity.state!="on") ? "mdi:clock-start" :
            "mdi:stop-circle-outline" ]]]
          tap_action:
            action: toggle
          variables:
            background_color: rgba(255, 87, 34, 0.2)
            color: '[[[ return (entity.state!="on") ? "orange" : "red" ]]]'
      timer:
        card:
          type: custom:button-card
          entity: "[[[ return variables.timer; ]]]"
          show_name: false
          show_icon: false
          show_state: '[[[ return (entity.state == "on");]]]'
          tap_action:
            action: none
          styles:
            card:
              - box-shadow: none
              - filter: opacity(50%)
              - border-width: 0
              - "--mdc-ripple-press-opacity": 0
              - padding-right: 10px;
            state:
              - font-size: 18px
            grid:
              - grid-template-areas: s
              - grid-template-columns: min-content
              - grid-template-rows: min-content
```
