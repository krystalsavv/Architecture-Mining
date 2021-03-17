import obs from "../Observer/observer.js"

let UIManager = {}; 

// ----------------------------------------------------------------------------------

UIManager.onChange = function(event, data) {
    obs.fire(event, data);
}


UIManager.onChangeSelect = function(event, data) {
    obs.fire(event + data);
}


UIManager.applyChanges = function(event, groupData, applyOnGroupData) {
    let data = {};

    function dataCreation(element, data, groupData) {
        if (typeof element === 'object' && element !== null) {
            let subData = {};
            Object.keys(element).forEach(key => {
                element[key].forEach(el => {
                    dataCreation(el, subData, groupData[key].data);
                    data[key] = subData;
                })
            });
        } else {
            data[element] = groupData[element].value;
        }
    }
    applyOnGroupData.forEach(element => {
        dataCreation(element, data, groupData);
    });
    obs.fire(event, data)
}

// ----------------------------------------------------------------------------------

UIManager.evalCondition = function(condition) {
    let $currGroup = this.group.data;
    return eval(' ' + condition);
}

// ----------------------------------------------------------------------------------

export default UIManager;