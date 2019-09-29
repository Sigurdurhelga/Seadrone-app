import React from 'react';

import styles from './CommandArea.css';

class CommandArea extends React.Component{

    constructor(props){
        super(props);
    }

    render(){
        return(
            <div className='command-wrapper'>
                <div className={"command-button " + (this.props.areaSelect ? "selected " : "")} onClick={this.props.selectArea}>Select Area</div>
                <div className={"command-button "} onClick={this.props.sendWaypoints}>Send Waypoints</div>
            </div>
        )
    }

}

export default CommandArea;