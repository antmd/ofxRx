#include "ofApp.h"

const int FLYING_DELAY_MS = 200;

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetCircleResolution(100);
    ofEnableSmoothing();
    
    message = "Time flies like an arrow";
    
    mouse.setup();
    keyboard.setup();
    updates.setup();
    gui.setup();

    //
    // calculate orbit position based on time
    //

    auto orbit_points = updates.milliseconds().
        map([this](unsigned long long tick){return ofMap(tick % int(orbit_period * 1000), 0, int(orbit_period * 1000), 0.0, 1.0);}).
        map([this](float t){return orbit_circle ? ofPoint(orbit_radius * std::cos(t * 2 * 3.14), orbit_radius * std::sin(t * 2 * 3.14)) : ofPoint();}).
        as_dynamic();

    //
    // use most recently selected source of points
    //

    auto center_points = center_source.get_observable().
        switch_on_next().
        as_dynamic();

    //
    // collect rolling window of past points
    //
    
    center_points.
        combine_latest(updates.milliseconds()).
        subscribe([=](const move_record& pt){
            move_window.push_back(pt);
            while(move_window.size() > 1 && std::get<1>(move_window.front()) < std::get<1>(pt) - (message.size() * FLYING_DELAY_MS)) {
                move_window.pop_front();
            }
        });

    //
    // adding current center to current orbit position results in a stream of points that orbit the center points.
    //

    orbit_points.
        combine_latest(std::plus<ofPoint>(), center_points).
        subscribe(
              [this](ofPoint c){
                  center = c;
              });

    //
    // define point sources
    //

    auto pointFromMouse = [](ofMouseEventArgs e){
        return ofPoint(e.x, e.y);
    };
    
    auto window_center = rx::observable<>::just(ofPoint((ofGetWidth()/2) - circle_radius, (ofGetHeight()/2) - circle_radius)).
        as_dynamic();
    
    auto all_movement = rx::observable<>::from(mouse.moves(), mouse.drags()).
        merge().
        map(pointFromMouse).
        as_dynamic();

    auto just_moves = mouse.moves().
        map(pointFromMouse).
        as_dynamic();

    auto just_drags = mouse.drags().
        map(pointFromMouse).
        as_dynamic();
    
    auto never = rx::observable<>::never<ofPoint>().
        as_dynamic();

    auto sources = rx::util::to_vector({window_center, all_movement, just_moves, just_drags, never});

    //
    // display gui
    //
    
    auto sourcesText = rx::util::to_vector<string>({"window_center", "all_movement", "just_moves", "just_drags", "never"});

    gui.add(show_circle.setup("circle", true));
    gui.add(orbit_circle.setup("circle orbits", true));
    gui.add(show_text.setup("flying text", false));
    gui.add(flyingText.setup("flying text", message));
    gui.add(circle_radius.setup("circle radius", 20.0, 10.0, 600.0));
	gui.add(orbit_radius.setup("orbit radius", 50.0, 10.0, 600.0));
    gui.add(orbit_period.setup("orbit period", 1.0, 0.5, 5.0));
	gui.add(selected.setup("select source", 0, 0, sources.size()));
    gui.add(selectedText.setup("selected source", sourcesText[0]));

    flyingText.setSize((13 + message.size()) * 8.5, flyingText.getHeight());
    selectedText.setSize((17 + sourcesText[0].size()) * 8.5, selectedText.getHeight());

    //
    // edit flying text
    //

    keyboard.releases().
        map([](ofKeyEventArgs a){return a.key;}).
        filter([](int key){ return key == OF_KEY_BACKSPACE; }).
        subscribe([this](int){
            if (!message.empty()) {
                message.erase(--message.end());
            }
            flyingText = message;
            flyingText.setSize((13 + message.size()) * 8.5, flyingText.getHeight());
        });

    keyboard.releases().
        map([](ofKeyEventArgs a){return a.key;}).
        filter([](int key){ return !(key & OF_KEY_MODIFIER) && key != OF_KEY_BACKSPACE; }).
        subscribe([this](char c){
            message.push_back(c);
            flyingText = message;
            flyingText.setSize((13 + message.size()) * 8.5, flyingText.getHeight());
        });

    //
    // switch source on selection changes
    //
    
    auto dest_center = center_source.get_subscriber().get_observer();

    dest_center.on_next(window_center);
    
    selections.setup(selected).
        subscribe([=](int selected){
            dest_center.on_next(sources[selected % sources.size()]);
            string msg = sourcesText[selected % sourcesText.size()];
            selectedText = msg;
            selectedText.setSize((17 + msg.size()) * 8.5, selectedText.getHeight());
        });
}

//--------------------------------------------------------------
void ofApp::draw(){

    ofBackgroundGradient(ofColor::white, ofColor::gray);

    ofFill();

    //
    // display circle
    //

	ofSetColor(ofColor(0x66,0x33,0x99));

    if (show_circle) {
        ofCircle(center, circle_radius);
    }

    //
    // display flying text
    //

    if (show_text && !move_window.empty()) {
        auto pt_cursor = move_window.begin();
        auto index = 0;
        auto now = ofGetElapsedTimeMillis();
        for (auto& c : message) {
            std::string s;
            s.push_back(c);
            ofPoint at;
            long tick;
            std::tie(at, tick) = move_window.front();
            auto time = now - (FLYING_DELAY_MS * index);
            auto found = std::find_if(move_window.rbegin(), move_window.rend(),
                [&](const move_record& tp){
                    return time > std::get<1>(tp);
                });
            if (found == move_window.rend()) {
                std::tie(at, tick) = move_window.back();
            } else {
                std::tie(at, tick) = *found;
            }
            ofDrawBitmapStringHighlight(s, at.x + (index * 15), at.y);
            ++index;
        }
    }

    gui.draw();
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}