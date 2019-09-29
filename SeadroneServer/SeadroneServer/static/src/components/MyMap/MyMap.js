import React from 'react';
import { Map, Marker, TileLayer, Popup } from 'react-leaflet';

class MyMap extends React.Component {

    constructor(props){
        super(props);

        this.state ={
            initialPosition: {
                lat: 64.122892,
                lng: -21.94725
            },
            zoom: 11,
        }
    }

    render() {
        let mapmarkers = Object.keys(this.props.markers).map((key, index) => {
            return <div className="marker" key={index}>{this.props.markers[key]}</div>
        })
        mapmarkers.push(this.props.boatMarker);
        return (
        <Map
            className="my-map"
            center={this.state.initialPosition}
            zoom={this.state.zoom}
            onClick={this.props.clickHandle}
            /*
            onMouseDown={this.props.clickHandle}
            */
            ref={m => {this.leafletMap = m}}>
            <TileLayer
                minZoom={2}
                maxZoom={16}
                attribution="the internet"
                url='OSMPublicTransport/{z}/{x}/{y}.png'
            />
            {mapmarkers}
        </Map>
        )
    }
}

export default MyMap;