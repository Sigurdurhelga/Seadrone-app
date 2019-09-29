import React from 'react';
import ReactDOM from 'react-dom';
import { Marker, Popup, L } from 'react-leaflet';

import TopBar from './components/TopBar/TopBar';
import MyMap from './components/MyMap/MyMap';
import CommandArea from './components/CommandArea/CommandArea';


import styles from './index.css';

class App extends React.Component {

    constructor(props) {
        super(props);

        this.state = {
            markerID: 0,
            markers: {},
            boatPosition: {
                lat: 0,
                lng: 0 
            },
            areaSelect: false
        }

        /*
        this.boatIcon = L.icon.extend({
            options: {
                iconUrl: require("../public/images/marker-icon-2x-red.png"),
                iconRetinaUrl: require("../public/images/marker-icon-2x-red.png"),
            }
        });
        */

        this.mapClickHandle = this.mapClickHandle.bind(this);
        this.selectArea = this.selectArea.bind(this);
        this.addMarker = this.addMarker.bind(this);
        this.sendWaypoints = this.sendWaypoints.bind(this);
    }


    componentDidMount() {
        this.updateBoatPosition();
        this.interval = setInterval(() => this.updateBoatPosition(), 3000);
    }

    updateBoatPosition() {
        fetch('http://localhost:5000/api/boatpos').then((response) => {
            if(!response.ok || response.status == 204){
                throw Error("didn't get boat position");
            } else {
                return response.json()
            }
        }).then((data) => {
            this.setState({
                boatPosition: {
                    lat: data.lat,
                    lng: data.lng
                }
            })
        }).catch((error) => {

        })
    }

    constructBoatMarker() {
        if(this.state.boatPosition != {}){
            return (
                <div className="marker" key={-1}>
                    <Marker position={this.state.boatPosition} autopan={false}>
                        <Popup>
                            <span>
                                Boat is here
                            </span>
                        </Popup>
                    </Marker>
                </div>
            )
        } else {
            return null;
        }
    }


    sendWaypoints(){
        let markerCoords = {}
        if(Object.keys(this.state.markers).length > 0){
            Object.keys(this.state.markers).map((key, index) => {
                markerCoords[index] = this.state.markers[key].props.position;
            })

            fetch('http://localhost:5000/api/command', {
                method: 'POST',
                headers: {'Content-Type':'application/json'},
                body: JSON.stringify({
                    "COMMAND_TYPE": 'WAYPOINT_FOLLOW',
                    "markers": markerCoords
                })
            });
        }
    }

    selectArea(){
        console.log("SELECTING AREA!");
        this.setState((prevState) => {
            return {
                areaSelect: !prevState.areaSelect
            }
        });
    }

    areaSelectHandle(event){
        console.log(event)
        event.originalEvent.preventDefault();
        console.log("HANDLING SELECTING AREA");
        event.originalEvent.stopPropagation();
        return false
    }

    mapClickHandle(event){
        console.log("handling click");
        if(this.state.areaSelect){
            this.areaSelectHandle(event)
        } else {
            this.addMarker(event)
        }
    }

    addMarker(event){
        let currID = this.state.markerID;
        let markerObj = (<Marker position={event.latlng} autopan={false} onClick={() => {
            this.setState((prevState) => {
                let newState = prevState.markers;
                delete newState[currID];
                return {
                    markers: newState
                }
            })
        }}> </Marker>)
        let newMarker = {};
        newMarker[currID] = markerObj;
        let newMarkers = Object.assign(this.state.markers, newMarker);
        this.setState((prevState) => {
            return {
                marker: newMarkers,
                markerID: prevState.markerID+1
            }
        })
    }



    render() {
        return (
            <div className="app-wrapper">
                <TopBar />
                <MyMap className="my-map" clickHandle={this.mapClickHandle}
                        markers={this.state.markers} boatMarker={this.constructBoatMarker()}/>
                <CommandArea className="command-area" 
                    selectArea={this.selectArea} areaSelect={this.state.areaSelect} 
                    sendWaypoints={this.sendWaypoints} />
            </div>
        );
    }
}


ReactDOM.render(<App />, document.getElementById('app'));
