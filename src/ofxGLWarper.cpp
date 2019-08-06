#include "ofxGLWarper.h"
#include "ofxHomography.h"

ofxGLWarper::~ofxGLWarper(){
    deactivate();
}

//--------------------------------------------------------------
void ofxGLWarper::setup(){
    setup(ofGetWidth(), ofGetHeight());
}
//--------------------------------------------------------------
void ofxGLWarper::setup(int _resX, int _resY){
    setup(0,0,_resX,_resY);
}
//--------------------------------------------------------------
void ofxGLWarper::setup(int _x, int _y, int _w, int _h){
    ofLogVerbose() << "ofxGLWarper setup: " <<_x << " " <<_y << " " <<_w << " " <<_h << endl;

    corners[TOP_LEFT] =     glm::vec2( _x      , _y        );
    corners[TOP_RIGHT] =    glm::vec2( _x + _w , _y        );
    corners[BOTTOM_RIGHT] = glm::vec2( _x + _w , _y + _h   );
    corners[BOTTOM_LEFT] =  glm::vec2( _x      , _y + _h   );

    deactivate(); // function checks if was already active

    myMatrix = glm::mat4(); // identity

    x=_x;
    y=_y;
    width=_w;
    height=_h;
    selectedCorner = -1;
    cornerIsSelected = false;
    cornerSensibility = 0.5;
    bUseKeys = true;
    bUseMouse = true;

	if(cornersPoly.size() != 4){
		cornersPoly.clear();
		for(int i = 0; i < 4; i++){
			cornersPoly.addVertex(corners[i]->x, corners[i]->y);
		}
		cornersPoly.close();
	}
	
    processMatrices();
}

//--------------------------------------------------------------
void ofxGLWarper::reSetupWarped(int _x, int _y, int _w, int _h){

    corners[TOP_LEFT] =     glm::vec2(fromScreenToWarpCoord( _x      , _y        ));
    corners[TOP_RIGHT] =    glm::vec2(fromScreenToWarpCoord( _x + _w , _y        ));
    corners[BOTTOM_RIGHT] = glm::vec2(fromScreenToWarpCoord( _x + _w , _y + _h   ));
    corners[BOTTOM_LEFT] =  glm::vec2(fromScreenToWarpCoord( _x      , _y + _h   ));

    x=_x;
    y=_y;
    width=_w;
    height=_h;

    processMatrices();
}

//--------------------------------------------------------------
bool ofxGLWarper::isActive(){
    return active;
}
//--------------------------------------------------------------
void ofxGLWarper::activate(bool bActivate){
    if (bActivate && !active){
        if (bUseMouse){ofRegisterMouseEvents(this);}
        if (bUseKeys) {ofRegisterKeyEvents(this);}
        active=true;

    }else if (!bActivate && active){
        if (bUseMouse){ofUnregisterMouseEvents(this);}
        if (bUseKeys) {ofUnregisterKeyEvents(this);}
        active=false;
    }
}
//--------------------------------------------------------------
void ofxGLWarper::deactivate(){
    activate(false);
}
//--------------------------------------------------------------
void ofxGLWarper::toggleActive(){
    activate(!active);
}
//--------------------------------------------------------------
void ofxGLWarper::enableKeys(bool k){
    if (bUseKeys != k){
        if (k && active) {
            ofRegisterKeyEvents(this);
        }else if (active){
            ofUnregisterKeyEvents(this);
        }
        bUseKeys = k;
    }
}
//--------------------------------------------------------------
void ofxGLWarper::toggleKeys(){
    enableKeys(!bUseKeys);
}
//--------------------------------------------------------------
void ofxGLWarper::enableMouse(bool m){
    if (bUseMouse != m){
        if (m && active) {
            ofRegisterMouseEvents(this);
        }else if (active){
            ofUnregisterMouseEvents(this);
        }
        bUseMouse = m;
    }
}
//--------------------------------------------------------------
void ofxGLWarper::toggleMouse(){
    enableMouse(!bUseMouse);
}
//--------------------------------------------------------------
void ofxGLWarper::processMatrices(){
    //we set it to the default - 0 translation
    //and 1.0 scale for x y z and w
    myMatrix = glm::mat4(); // default constructor generates identity

    glm::vec2 cvsrc[4];
    glm::vec2 cvdst[4]; //corners is our destination matrix, but we can't pass an ofParameter to findHomography. So we copy.
    //if you know a way though ...

    //we set the warp coordinates
    //source coordinates as the dimensions of our window
    cvsrc[0] = glm::vec2(0,0);
    cvsrc[1] = glm::vec2(width,0);
    cvsrc[2] = glm::vec2(width,height);
    cvsrc[3] = glm::vec2(0,height);

    //corners are in 0.0 - 1.0 range
    //so we scale up so that they are at the window's scale
    for(int i = 0; i < 4; i++){
        cvdst[i] = corners[i];
    }


    //figure out the warping!
    myMatrix = ofxHomography::findHomography(cvsrc, cvdst);
	
	
	if(cornersPoly.size() == 4){
	
		auto& v = cornersPoly.getVertices();
		for(int i = 0; i < 4; i++){
			v[i] =  {corners[i]->x, corners[i]->y, 0};
		}
	}else{
		ofLogError("ofxGLWarper::processMatrices") << "cornersPoly size is not 4. Probably setup() was not called!";
	}

}
//--------------------------------------------------------------
void ofxGLWarper::draw(){ // Deprecated (included in end()). Please check the drawSettings structure.
}
//--------------------------------------------------------------
void ofxGLWarper::begin(){
    ofPushMatrix();
    ofMultMatrix(myMatrix);
    ofTranslate(-1*x,-1*y); //if not, you have to draw from warper's origin. As you wish.
}
//--------------------------------------------------------------
void ofxGLWarper::end(){
    if ((drawSettings.bDrawRectangle && active) || (drawSettings.bDrawRectangle && drawSettings.bForceDrawing)) {
        ofPushStyle();
        ofSetColor(drawSettings.rectangleColor);
        ofNoFill();
        ofDrawRectangle(x, y, width, height);
    }
    ofPopMatrix();
    if ((drawSettings.bDrawCorners && active) || (drawSettings.bDrawCorners && drawSettings.bForceDrawing)) {// this draws colored squares over the corners as a visual aid.
        ofSetRectMode(OF_RECTMODE_CENTER);
        for (int i = 0; i < 4; i++) {
            if(i==selectedCorner){
                ofSetColor(drawSettings.selectedCornerColor);
            }else{
                ofSetColor(drawSettings.cornersColor);
            }
            ofFill();
            ofDrawRectangle(corners[i], 10, 10);
        }
        ofPopStyle();
    }
}
//--------------------------------------------------------------
void ofxGLWarper::save(const string &saveFile){
    ofXml XML;
    saveToXml(XML);
    XML.save(saveFile);
}
//--------------------------------------------------------------
void ofxGLWarper::saveToXml(ofXml &XML, const string& warperID){
    
    XML.removeChild(warperID);//if child doesn't exist yet, it's ok.
    auto c = XML.appendChild(warperID);
    for(int i =0; i<4; i++){
		auto nc = c.appendChild("corner");
        nc.appendChild("x").set(corners[i]->x);
        nc.appendChild("y").set(corners[i]->y);
    }
    c.appendChild("active").set(active);
}
//--------------------------------------------------------------
void ofxGLWarper::load(const string &loadFile){
    ofXml XML;
    if( !XML.load(loadFile) ){
        ofLog(OF_LOG_ERROR, "ofxGLWarper : xml file not loaded. Check file path.");
        return;
    }
    loadFromXml(XML);
}

//--------------------------------------------------------------
void ofxGLWarper::loadFromXml(ofXml &XML, const string& warperID){
    auto c = XML.getChild(warperID);
    if(!c){
        ofLog(OF_LOG_ERROR, "ofxGLWarper : incorrrect xml formating. No \"" + warperID + "\" tag found");
        return;
    }
	
    if (c.find("corner").size()<4 ) {
        ofLog(OF_LOG_ERROR, "ofxGLWarper : incorrrect xml formating. less than 4 \"corner\" tags found");
        return;
    }
	auto cor = c.getChildren("corner");
	int i = 0;
	for(auto& ch: cor){
        if(i<4){
            corners[i] = glm::vec2(ch.getChild("x").getFloatValue(), ch.getChild("y").getFloatValue());
        } 
		i++;
    }

    this->activate(c.getChild("active").getBoolValue());

    processMatrices();
    //ofLog(OF_LOG_WARNING, "ofxGLWarper : xml object loaded OK!."); // Since the method works, this can be quiet...

}
//--------------------------------------------------------------
void ofxGLWarper::mouseDragged(ofMouseEventArgs &args){
	if(!ofGetKeyPressed(OF_KEY_SHIFT) && bMoveAll){
		bMoveAll = false;
	}
	if(bMoveAll){
		moveAllCorners(args - mousePressPos);
		mousePressPos = args;
	}else{
	if(cornerIsSelected && selectedCorner >= 0){
        corners[selectedCorner] = glm::vec2(args.x, args.y);
    }
	}
    processMatrices();
}
//--------------------------------------------------------------
void ofxGLWarper::mousePressed(ofMouseEventArgs &args){
	if(ofGetKeyPressed(OF_KEY_SHIFT) && cornersPoly.inside(args.x, args.y)){
		bMoveAll = true;
		mousePressPos = args;
	}else{
		bMoveAll = false;
    float smallestDist = sqrtf(ofGetWidth() * ofGetWidth() + ofGetHeight() * ofGetHeight());
    //selectedCorner = -1;
    float sensFactor = cornerSensibility * sqrtf( width  * width  + height  * height );

    cornerIsSelected = false;
    for(int i = 0; i < 4; i++){
        float distx = corners[i]->x - args.x;
        float disty = corners[i]->y - args.y;
        float dist  = sqrtf( distx * distx + disty * disty);
        ofLogVerbose() << "mouse to corner dist: " << dist << endl;
        if(dist < smallestDist && dist < sensFactor ){
            selectedCorner = i;
            smallestDist = dist;
            cornerIsSelected=true;
        }
    }
	}
}
//--------------------------------------------------------------
void ofxGLWarper::mouseReleased(ofMouseEventArgs &args){
//	cornerIsSelected = false;
}
//--------------------------------------------------------------
void ofxGLWarper::keyPressed(ofKeyEventArgs &args){

    if (cornerIsSelected && selectedCorner >= 0) {
        switch (args.key) {
            case OF_KEY_DOWN:
                corners[selectedCorner] += glm::vec2(0,1);
                processMatrices();
                break;
            case OF_KEY_UP:
                corners[selectedCorner] += glm::vec2(0,-1);
                processMatrices();
                break;
            case OF_KEY_LEFT:
                corners[selectedCorner] += glm::vec2(-1,0);
                processMatrices();
                break;
            case OF_KEY_RIGHT:
                corners[selectedCorner] += glm::vec2(1,0);
                processMatrices();
                break;
            default:
			break;
		}
	}
}
//--------------------------------------------------------------
void ofxGLWarper::keyReleased(ofKeyEventArgs &args){
	
}
 //--------------------------------------------------------------
glm::vec4 ofxGLWarper::fromScreenToWarpCoord(float x, float y, float z){
    glm::vec4 mousePoint;
    glm::vec4 warpedPoint;

    // this is the point on the image which i want to know the coordinates inside the warped system ...
    mousePoint.x = x - (this->x);
    mousePoint.y = y - (this->y);
    mousePoint.z = z;
    mousePoint.w = 1.0;

    // with glm matrices, no need to transpose and inverse, because if you have a mat4 and do mat4*vec4,
    // your vec4 is considered a column vector. If you do vec4*mat4, it is considered a row vector.

    // multiply both to get the point transformed by the matrix
    warpedPoint = myMatrix * mousePoint ;

    // we need to normalize the value as described here : http://tech.groups.yahoo.com/group/OpenCV/message/80121
    warpedPoint.x = warpedPoint.x / warpedPoint.w;
    warpedPoint.y = warpedPoint.y / warpedPoint.w;
    warpedPoint.z = warpedPoint.z / warpedPoint.w;

    return warpedPoint;
}
glm::vec4 ofxGLWarper::fromScreenToWarpCoord(const glm::vec4 &position){
    return fromScreenToWarpCoord(position.x, position.y, position.z);
}
 //--------------------------------------------------------------
glm::vec4 ofxGLWarper::fromWarpToScreenCoord(float x, float y, float z){
    glm::vec4 mousePoint;
    glm::vec4 warpedPoint;

    // this is the point inside the warped system which i want to know the coordinates on the image  ...
    mousePoint.x = x;
    mousePoint.y = y;
    mousePoint.z = z;
    mousePoint.w = 1.0;

    // invert the matrix
    glm::mat4 invertedMyMatrix = glm::inverse(myMatrix);

    // multiply both to get the point transformed by the matrix
    warpedPoint = invertedMyMatrix * mousePoint ;

    warpedPoint.x = warpedPoint.x / warpedPoint.w + (this->x);
    warpedPoint.y = warpedPoint.y / warpedPoint.w + (this->y);
    warpedPoint.z = warpedPoint.z / warpedPoint.w;

    return warpedPoint;
}
glm::vec4 ofxGLWarper::fromWarpToScreenCoord(const glm::vec4 &position){
    return fromWarpToScreenCoord(position.x, position.y, position.z);
}

//--------------------------------------------------------------
void ofxGLWarper::setAllCorners(const glm::vec2 &top_left, const glm::vec2 &top_right, const glm::vec2 &bot_left, const glm::vec2 &bot_right){
    //if you want to set all corners and avoid 3 useless processMatrices()
    corners[TOP_LEFT] = top_left;
    corners[TOP_RIGHT] = top_right;
    corners[BOTTOM_LEFT] = bot_left;
    corners[BOTTOM_RIGHT] = bot_right;

    processMatrices();
}
//--------------------------------------------------------------
void ofxGLWarper::moveAllCorners(const glm::vec2 &moveBy){
    for (int i = 0; i < 4; ++i) {
        corners[i] += moveBy;
    }
    processMatrices();
}
void ofxGLWarper::moveAllCorners(float byX, float byY){
    glm::vec2 moveBy(byX,byY);
    moveAllCorners(moveBy);
}
//--------------------------------------------------------------
void ofxGLWarper::selectCorner(CornerLocation cornerLocation){
    selectedCorner = cornerLocation;
}
//--------------------------------------------------------------
void ofxGLWarper::setCorner(CornerLocation cornerLocation, const glm::vec2 &onScreenLocation){
    corners[cornerLocation] = onScreenLocation;// glm::vec2(width, height);
    processMatrices();
}
//--------------------------------------------------------------
void ofxGLWarper::setCorner(CornerLocation cornerLocation, float onScreenLocationX, float onScreenLocationY){
    corners[cornerLocation] = glm::vec2(onScreenLocationX, onScreenLocationY);// glm::vec2(width, height);
    processMatrices();
}
//--------------------------------------------------------------
void ofxGLWarper::moveCorner(CornerLocation cornerLocation, const glm::vec2 &moveBy){
        corners[cornerLocation] += moveBy;
        processMatrices();
}
//--------------------------------------------------------------
void ofxGLWarper::moveCorner(CornerLocation cornerLocation, float byX, float byY){
    glm::vec2 moveBy(byX,byY);
    moveCorner(cornerLocation, moveBy);
}
//--------------------------------------------------------------
glm::vec2 ofxGLWarper::getCorner(CornerLocation cornerLocation){
    return corners[cornerLocation];// * glm::vec2(width, height);
}
//--------------------------------------------------------------
void ofxGLWarper::setCornerSensibility(float sensibility){
    cornerSensibility = sensibility;
}
//--------------------------------------------------------------
float ofxGLWarper::getCornerSensibility(){
    return cornerSensibility;
}
//--------------------------------------------------------------
ofRectangle ofxGLWarper::getBaseRectangle(){
	return ofRectangle(x,y,width,height); // gets you the rect used to setup
}
//--------------------------------------------------------------
bool ofxGLWarper::getCornerIsSelected(){
	return cornerIsSelected && selectedCorner >= 0;
}
//--------------------------------------------------------------
// When no corner is selected ( getCornerIsSelected() == false ) :
// getSelectedCornerLocation() < static_cast<ofxGLWarper::CornerLocation>(0)
ofxGLWarper::CornerLocation ofxGLWarper::getSelectedCornerLocation(){
	ofxGLWarper::CornerLocation corner_loc = static_cast<ofxGLWarper::CornerLocation>(selectedCorner);
	return corner_loc;
}
