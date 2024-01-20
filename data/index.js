// @title     StarMod
// @file      index.css
// @date      20240114
// @repo      https://github.com/ewowi/StarMod
// @Authors   https://github.com/ewowi/StarMod/commits/main
// @Copyright (c) 2024 Github StarMod Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

let d = document;
let ws = null;

let mdlColumnNr = 0;
let nrOfMdlColumns = 4;
let jsonValues = {};
let uiFunCommands = [];
let model = []; //model.json (as send by the server), used by FindVar
let savedView = null;

function gId(c) {return d.getElementById(c);}
function cE(e) { return d.createElement(e); }

function handleVisibilityChange() {
  console.log("handleVisibilityChange");
}

function onLoad() {
  getTheme();

  makeWS();

  initMdlColumns();

  d.addEventListener("visibilitychange", handleVisibilityChange, false);
}

function makeWS() {
  if (ws) return;
  let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
  console.log("makeWS url", url);
  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onmessage = (e)=>{
    if (e.data instanceof ArrayBuffer) { // preview packet
      userFun(e.data);
    } 
    else {
      // console.log("onmessage", e.data);
      clearTimeout(jsonTimeout);
      jsonTimeout = null;
      gId('connind').style.backgroundColor = "var(--c-l)";

      // console.log("onmessage", e.data);
      let json = null;
      try {
        json = JSON.parse(e.data);
      } catch (error) {
          json = null;
          console.error("makeWS json error", error, e.data); // error in the above string (in this case, yes)!
      }
      if (json) {
        //receive model per module to stay under websocket size limit of 8192
        if (json.type && ["appmod","usermod", "sysmod"].includes(json.type)) { //generate array of variables
          let found = false;
          for (let module of model) {
            if (module.id == json.id)
              found = true;
          }
          if (!found) {
            let module = json;
            model.push((module)); //this is the model
            console.log("WS receive generateHTML", module);
            generateHTML(module); //no parentNode

            if (module.id == "System") {
              console.log("system changes", module);
              if (module.view)
                savedView = module.view;
              if (module.theme)
                changeHTMLTheme(module.theme);
            }

            //rerun after each module added
            if (savedView)
              changeHTMLView(savedView);
            else
              changeHTMLView("vApp"); //default

            //send request for uiFun
            flushUIFunCommands();
          }
          else
            console.log("html of module already generated", json);
        }
        else { //update
          if (!Array.isArray(json)) //only the model is an array
            // console.log("WS receive update", json);
            receiveData(json);
          else
            console.log("program error array not expected", json);
        }
      }
    }
  }
  ws.onclose = (e)=>{
    console.log("WS close and retry", e);
    gId('connind').style.backgroundColor = "var(--c-r)";
    setTimeout(makeWS,1500); // retry WS connection
    ws = null;
  }
  ws.onopen = (e)=>{
    console.log("WS open", e);
		reqsLegal = true;
  }
  ws.onerror = (e)=>{
    console.log("WS error", e);
  }
}

function linearToLogarithm(json, value) {
  if (value == 0) return 0;

  var minp = json.min?json.min:0;
  var maxp = json.max?json.max:255;

  // The result should be between 100 an 10000000
  var minv = minp?Math.log(minp):0;
  var maxv = Math.log(maxp);

  // calculate adjustment factor
  var scale = (maxv-minv) / (maxp - minp);

  let result = Math.exp(minv + scale*(value-minp));

  // console.log(json, minv, maxv, scale, result);

  return Math.round(result);
}

function generateHTML(json, parentNode = null, rowNr = -1) {

  // console.log("generateHTML", json, parentNode);
  if (Array.isArray(json)) {
    //sort according to o value
    json.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });

    for (let variable of json) { //if isArray then variables of array
      generateHTML(variable, parentNode, rowNr);
    }
  }
  else { // json is variable
    let  variable = json;

    //if root (type module) add the html to one of the mdlColumns
    if (parentNode == null) {
      parentNode = gId("mdlColumn" + mdlColumnNr);
      mdlColumnNr = (mdlColumnNr +1)%nrOfMdlColumns; //distribute over columns (tbd: configure)
    }
    let parentNodeType = parentNode.nodeName.toLocaleLowerCase();

    let isPartOfTableRow = (rowNr != -1);

    // if (!variable || !variable.id) {
    //   console.log("genHTML no variable and id", variable, parentNode); //tbd: caused by more data then columns in table...
    //   return;
    // }

    let divNode = null; //divNode will be appended to the parentNode after if then else and returned
    let varNode = null; //the node containing the variable
    let rangeValueNode = null;
    // let buttonSaveNode = null;
    // let buttonCancelNode = null;

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
       
    let labelNode = cE("label");
    labelNode.innerText = initCap(variable.id); // the default when not overridden by uiFun
    
    divNode = cE("div");
    divNode.id = variable.id + (isPartOfTableRow?"#" + rowNr:"") + "_d";

    //table cells and buttons don't get a label
    if (parentNodeType != "td") {
      if (variable.type != "button" && !["appmod","usermod", "sysmod"].includes(variable.type)) divNode.appendChild(labelNode); //add label (tbd:must be done by childs n table cell)
    }

    if (["appmod","usermod", "sysmod"].includes(variable.type)) { //if module
      ndivNeeded = false;

      varNode = cE("div");
      let h2Node = cE("h2");
      h2Node.innerText = initCap(variable.id);
      varNode.appendChild(h2Node);

      let helpNode = cE("input");
      helpNode.type = "button";
      helpNode.value = "?";
      helpNode.addEventListener('click', (event) => {
        location.href="https://ewowi.github.io/StarDocs/";// + variable.id;
        // location.href="https://starmod.org/" + variable.id;
      });
      varNode.appendChild(helpNode);

      setupModule(varNode); //enable drag and drop of modules
    }
    else if (variable.type == "table") {
      ndivNeeded = false;

      //add table
      varNode = cE("table");

      let theadNode = cE("thead");
      theadNode.appendChild(cE("tr"));
      varNode.appendChild(theadNode); //row for header

      let tbodyNode = cE("tbody");
      varNode.appendChild(tbodyNode);

      if (!variable.ro) {
        let buttonNode = cE("input");
        buttonNode.type = "button";
        buttonNode.value = "+";
        buttonNode.addEventListener('click', (event) => {
          let divNode = event.target.parentNode; //parent of the + button
          let tableNode = divNode.querySelector("table");
          let tbodyNode = tableNode.querySelector("tbody");
          console.log("Table +", divNode, variable, tableNode);

          let newRowNr = tbodyNode.querySelectorAll("tr").length;

          genTableRowHTML(variable, tableNode, newRowNr);

          var command = {};
          command["addRow"] = {"id": variable.id, "rowNr":newRowNr};
          requestJson(command);
        });
        divNode.appendChild(buttonNode);
      }

      //variable.n will add the columns
    }
    else if (parentNodeType == "table") { 
      // console.log("tableChild", parentNode, variable);

      varNode = cE("th");
      varNode.innerText = initCap(variable.id); //label uiFun response can change it

    } else if (variable.type == "select") {

      if (variable.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
        varNode = cE("span");
      }
      else {
        varNode = cE("select");
        varNode.addEventListener('change', (event) => {console.log("select change", event);sendValue(event.target);});
      }
    }
    else if (variable.type == "canvas") {
      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("canvas");
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }
    else if (variable.type == "textarea") {

      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("textarea");
      varNode.readOnly = variable.ro;
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }
    else if (variable.type == "url") {
      varNode = cE("a");
      // varNode.setAttribute('target', "_blank"); //does not work well on mobile
    } else if (variable.type == "checkbox") {
      varNode = cE("input");
      varNode.type = variable.type;
      varNode.disabled = variable.ro;
      varNode.indeterminate = true; //until it gets a value;
      varNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
    } else if (variable.type == "button") {
      varNode = cE("input");
      varNode.type = variable.type;
      varNode.disabled = variable.ro;
      varNode.value = initCap(variable.id); //initial label
      varNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);sendValue(event.target);});
    } else if (variable.type == "range") {
      varNode = cE("input");
      varNode.type = variable.type;
      varNode.min = variable.min?variable.min:0;
      varNode.max = variable.max?variable.max:255; //range slider default 0..255
      varNode.disabled = variable.ro;
      //numerical ui value changes while draging the slider (oninput)
      let rvNode = variable.id + (isPartOfTableRow?"#" + rowNr:"") + "_rv";
      varNode.addEventListener('input', (event) => {
        if (gId(rvNode)) {
          gId(rvNode).innerText = variable.log?linearToLogarithm(variable, event.target.value):event.target.value;
        }
      });
      //server value changes after draging the slider (onchange)
      varNode.addEventListener('change', (event) => {
        sendValue(event.target);
      });
      rangeValueNode = cE("span");
      rangeValueNode.id = rvNode; //rangeValue
    } else if (variable.type == "coord3D") {
        varNode = cE("div");
        let xNode = cE("input");
        xNode.type = "number";
        xNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
        if (variable.max) xNode.max = variable.max;
        xNode.placeholder = "x";
        let yNode = xNode.cloneNode();
        yNode.placeholder = "y";
        let zNode = xNode.cloneNode();
        zNode.placeholder = "z";
        varNode.appendChild(xNode);
        varNode.appendChild(yNode);
        varNode.appendChild(zNode);
    } else {
      //input types: text, search, tel, url, email, and password.

      if (variable.ro && variable.type != "button") {
        varNode = cE("span");
      } else {
        varNode = cE("input");
        varNode.type = variable.type;
        varNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
        // if (["text", "password", "number"].includes(variable.type) ) {
        //   buttonSaveNode = cE("text");
        //   buttonSaveNode.innerText = "✅";
        //   buttonSaveNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
        //   buttonCancelNode = cE("text");
        //   buttonCancelNode.innerText = "🛑";
        //   buttonCancelNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
        // }
        if (variable.type == "number") {
          varNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
          if (variable.max) varNode.max = variable.max;
        }
        else {
          if (variable.max) varNode.setAttribute('maxlength', variable.max); //for text and textarea set max length varNode.maxlength is not working for some reason
        }
      }
    } //if variable type

    if (parentNodeType == "table") { //table headers don't have a divNode (why not...)
      // divNode = varNode;
      parentNode.querySelector('thead').querySelector("tr").appendChild(varNode); //<thead><tr> (containing th)
    } else {
      divNode.appendChild(varNode);
      parentNode.appendChild(divNode);
    }
    varNode.id = variable.id + (isPartOfTableRow?"#" + rowNr:"");
    varNode.className = variable.type;

    if (rangeValueNode) divNode.appendChild(rangeValueNode); //_rv value of range / sliders
    // if (buttonSaveNode) divNode.appendChild(buttonSaveNode);
    // if (buttonCancelNode) divNode.appendChild(buttonCancelNode);
    
    //disable drag of parent module
    if (["appmod","usermod", "sysmod"].includes(variable.type)) {
      varNode.draggable = true;
      varNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
    }

    if (variable.n && parentNodeType != "table") { //multiple details, not for table header
      //add a div with _n extension and details have this as parent
      if (ndivNeeded) {
        let ndivNode = cE("div");
        ndivNode.id = variable.id + (isPartOfTableRow?"#" + rowNr:"") + "_n";
        ndivNode.className = "ndiv";
        divNode.appendChild(ndivNode); // add to the parent of the node
        generateHTML(variable.n, ndivNode, rowNr);
      }
      else
        generateHTML(variable.n, varNode, rowNr); //details (e.g. module)
    }

    //don't call uiFun on table rows (the table header calls uiFun and propagate this to table row columns in changeHTML when needed - e.g. select)
    if (variable.uiFun == null || variable.uiFun == -2) { //request processed
      variable.chk = "gen2";
      changeHTML(variable, varNode, variable, rowNr); // set the variable with its own changed values
    }
    else { //uiFun
      if (variable.value)
        changeHTML(variable, varNode, {"value":variable.value, "chk":"gen1"}, rowNr); //set only the value

      //call ui Functionality, if defined (to set label, comment, select etc)
      if (variable.uiFun >= 0) { //>=0 as element in var
        uiFunCommands.push(variable.id);
        if (uiFunCommands.length > 8) { //every 8 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushUIFunCommands();
        }
        variable.uiFun = -1; //requested
      }
    }

    return varNode;
  } //not an array but variable
}

function genTableRowHTML(json, parentNode = null, rowNr = -1) {
  let variable = json;
  let tbodyNode = parentNode.querySelector("tbody");
  // console.log("genTableRowHTML", parentNode.id, rowNr, tbodyNode.querySelectorAll("tr").length);

  //create a new row on the table
  let trNode = cE("tr");
  tbodyNode.appendChild(trNode);
  //genHTML for var(n)
  for (let columnVar of variable.n) {
    let tdNode = cE("td");
    trNode.appendChild(tdNode);
    generateHTML(columnVar, tdNode, rowNr); //will also do the values
  }
  if (!variable.ro) {
    let tdNode = cE("td");
    let buttonNode = cE("input");
    buttonNode.id = variable.id + "#" + rowNr + "_del";
    buttonNode.type = "button";
    buttonNode.value = "-";
    buttonNode.addEventListener('click', (event) => {
      console.log("Table -", event.target);

      var command = {};
      command["delRow"] = {"id": variable.id, "rowNr":rowNr};
      requestJson(command);

    });
    tdNode.appendChild(buttonNode);
    trNode.appendChild(tdNode);
  }
  flushUIFunCommands();
}

//process json from server, json is assumed to be an object
function receiveData(json) {
  // console.log("receiveData", json);

  if (Object.keys(json)) {
    for (let key of Object.keys(json)) {
      let value = json[key];

      //tbd: for each node of a variable (rowNr)

      //special commands
      if (key == "uiFun") {
        console.log("receiveData no action", key, value); //should not happen anymore
      }
      else if (key == "view") {
        console.log("receiveData", key, value);
        changeHTMLView(value);
      }
      else if (key == "theme") {
        console.log("receiveData", key, value);
        changeHTMLTheme(value);
      }
      else if (key == "canvasData") {
        console.log("receiveData no action", key, value);
      }
      else if (key == "details") {
        let variable = value;
        let rowNr = variable["rowNr"]!=null?variable["rowNr"]:-1;
        let nodeId = variable.id + ((rowNr != -1)?"#" + rowNr:"");
        //if var object with .n, create .n (e.g. see setEffect and fixtureGenChFun, tbd: )
        console.log("receiveData details", key, variable);
        if (gId(nodeId + "_n")) gId(nodeId + "_n").remove(); //remove old ndiv

        //create new ndiv
        if (variable.n) {
          let ndivNode = cE("div");
          ndivNode.id = nodeId + "_n";
          ndivNode.className = "ndiv";
          gId(nodeId).parentNode.appendChild(ndivNode);
          generateHTML(variable.n, ndivNode, rowNr);
        }
        flushUIFunCommands(); //make sure uiFuns of new elements are called
      }
      else if (key == "updRow") { //update the row of a table
        for (let tableId of Object.keys(value)) { //currently only one table
          let tableRows = value[tableId];
          console.log("receiveData updRow", key, tableId, tableRows);
          let tableNode = gId(tableId);
          let tableVar = findVar(tableId);
          // console.log("updRow main", tableId, tableRows, tableNode, tableVar);

          let rowFound = false;
          let rowNr = -1;
          for (let nodeRowNr = 1, rowNode; rowNode = tableNode.rows[nodeRowNr]; nodeRowNr++) { //<table> rows starting with header row
            rowNr = nodeRowNr - 1;
            // console.log("  noderow", rowNr, rowNode);

            if (Array.isArray(tableRows)) {
              for (let tableRow of tableRows) {
                // console.log("  tablerow", tableId, tableRow);

                //loop over all column vars
                //like genTableRowHTML but here only values, id's do exist already
                let colNr = 0;
                let keyFound = false; //reset each iteration
                for (let colVar of tableVar.n) {
                  let colNode = gId(colVar.id + "#" + rowNr); 
                  if (colNode) {
                    let colValue = tableRow[colNr];
                    // console.log("    ", colVar, colNode, colValue);
  
                    if (colNr == 0) { //check on the value of the first table column: tbd: check other columns?
                      keyFound = colNode.innerText == colValue; //innerText is assuming span like node. tbd: others
                    } else if (keyFound) { //colNr 1..n
                      rowFound = true;
                      // console.log("receiveData updRow, existing row", tableVar, tableNode, colVar, colNode, rowNr);
                      changeHTML(colVar, colNode, {"value":colValue, "chk":"updRow"}, rowNr);
                    }
                  }
                  else
                    console.log("receiveData node not found", colVar.id + "#" + rowNr, colVar);
                  colNr++;
                }
              }
            }
          } //for each row
          if (!rowFound) {
            // this is a new rode, add it
            console.log("receiveData updRow, new row", tableVar, tableNode, rowNr);
            // genTableRowHTML(tableVar, tableNode, rowNr+1);
          }
        } //tableId
      }
      else { //{variable:{label:value}}
        let variable = findVar(key);

        if (variable) {
          variable.uiFun = -2; // request processed

          if (gId(key)) { //update the variable and in case of a table the tableheader
            value.chk = "uiFun";
            changeHTML(variable, gId(key), value);
          }
          else
            console.log("receiveData id not found in dom", key, value);
        }
        else
          console.log("receiveData key is no variable", key, value);
      }
    } //for keys
  } //isObject
  else
    console.log("receiveData no Object", object);
} //receiveData

//do something with an existing (variable) node, key is an existing node, json is what to do with it
function changeHTML(variable, node, commandJson, rowNr = -1) {

  if (!node)
    console.log("changeHTML no node !", variable, node, commandJson, rowNr);

  let nodeType = node.nodeName.toLocaleLowerCase();
  let isPartOfTableRow = (rowNr != -1);

  if (commandJson.hasOwnProperty("label")) {
    if (nodeType == "th") //table header
      node.innerText = initCap(commandJson.label);
    else if (node.className == "button") {
      node.value = initCap(commandJson.label);
    }
    else {
      let labelNode = gId(node.id).parentNode.querySelector("label");
      if (labelNode) labelNode.innerText = initCap(commandJson.label);
    }

    variable.label = commandJson.label;
  } //label

  if (commandJson.hasOwnProperty("comment")) {
    
    if (nodeType != "th") {

      //if not a tablecell
      if (node.parentNode.parentNode.nodeName.toLocaleLowerCase() != "td") {
        let commentNode = node.parentNode.querySelector('comment');
        // console.log("commentNode", commentNode);
        if (!commentNode) { //create if not exist
          commentNode = cE("comment");
          node.parentNode.appendChild(commentNode);
        }
        commentNode.innerText = commandJson.comment;
      }
    }
    else { //th
      // console.log("changeHTML comment", variable, node, commandJson, rowNr);
      let ttdivNode = cE("div");
      ttdivNode.innerText = node.innerText;
      node.innerText = "";
      ttdivNode.classList.add("tooltip");
      let spanNode = cE("span");
      spanNode.innerHTML = commandJson.comment;
      spanNode.classList.add("tooltiptext");
      ttdivNode.appendChild(spanNode);

      node.appendChild(ttdivNode);

    }
    variable.comment = commandJson.comment;
  } //comment

  if (commandJson.hasOwnProperty("options")) { //replace the body of a table
    // console.log("changeHTML options", variable, node, commandJson, rowNr);

    if (nodeType == "select") { //span/ro will be set in .value
      //remove all old options first
      var index = 0;
      while (node.options && node.options.length > 0) {
        node.remove(0);
      }
      for (var value of commandJson.options) {
        let optNode = cE("option");
        if (Array.isArray(value)) {
          optNode.value = value[0];
          optNode.text = value[1];
        }
        else {
          optNode.value = index;
          optNode.text = value;
        }
        node.appendChild(optNode);
        index++;
      }
    }
      
    variable.options = commandJson.options;

    //if no new value, set the old one
    if (commandJson.value == null)
      changeHTML(variable, node, {"value":variable.value, "chk":"options"}, rowNr); //(re)set the select value
    // else
    //   console.log("changeHTML value will be set in value", variable, node, commandJson, rowNr);

  } //options

  if (commandJson.hasOwnProperty("value")) { 
    //hasOwnProperty needed to catch also boolean commandJson.value when it is false !!!!
    
    // if (node.id=="insName#0" || node.id=="fx")// || node.id=="mdlEnabled" || node.id=="clIsFull" || node.id=="pin2")
    //   console.log("changeHTML value", variable, node, commandJson, rowNr);
    if (nodeType == "table") {
      if (Array.isArray(commandJson.value)) {
        //remove table rows
        let tbodyNode = cE('tbody'); //the tbody of node will be replaced
        //replace the table body
        node.replaceChild(tbodyNode, node.querySelector("tbody")); //replace <table><tbody> by tbodyNode  //add to dom asap

        //add each row
        let newRowNr = 0;
        for (var row of commandJson.value) {
          genTableRowHTML(variable, node, newRowNr);
          let colNr = 0;
          for (let columnVar of variable.n) {
            let varId = columnVar.id + "#" + newRowNr;
            changeHTML(columnVar, gId(varId), {"value": row[colNr], "chk":"table"}, newRowNr);
            colNr++;
          }

          newRowNr++;
        }

        flushUIFunCommands(); //make sure uiFuns of new elements are called

        if (variable.id == "insTbl")
          setInstanceTableColumns();
      }
      else
        console.log("changeHTML value table no array", variable, node, commandJson, rowNr);
    }
    else if (nodeType == "th") {  //node.parentNode = table... updCol update column

      let tableNode = node.parentNode.parentNode.parentNode;
      let trNodes = tableNode.querySelector('tbody').querySelectorAll("tr");
      let tableVar = findVar(tableNode.id); //tbd: table in table
      let valueLength = Array.isArray(commandJson.value)?commandJson.value.length:1; //tbd: use table nr of rows (not saved yet)
      // console.log("changeHTML th column", node.id, (rowNr==-1)?JSON.stringify(commandJson.value):commandJson.value[rowNr], commandJson.chk, rowNr);

      let max = Math.max(valueLength, trNodes.length);
      for (let newRowNr = 0; newRowNr<max;newRowNr++) {
        let newValue; // if not array then use the value for each row
        if (Array.isArray(commandJson.value))
          newValue = commandJson.value[newRowNr];
        else
          newValue = commandJson.value;

        //if row not exists, create table row
        if (newRowNr > trNodes.length - 1) {
          genTableRowHTML(tableVar, tableNode, newRowNr); //this will set the whole row and its (default) values as stored in the model
        }
        else {
          //find the new table cell and change it's value
          let cellNode = gId(node.id + "#" + newRowNr);
          if (cellNode) {
            // console.log("changeHTML th cellNode found", cellNode.id, newRowNr);
            if (newRowNr < valueLength)
              changeHTML(variable, cellNode, {"value":newValue, "chk":"column"}, newRowNr);
            else
              changeHTML(variable, cellNode, {"value":null, "chk":"column"}, newRowNr);
          }
          else
            console.log("changeHTML th cellNode not found", node, node.id + "#" + newRowNr);
        }
        // newRowNr++;
      }

      flushUIFunCommands(); //make sure uiFuns of new elements are called

    }
    else if (node.parentNode.parentNode.nodeName.toLocaleLowerCase() == "td" && Array.isArray(commandJson.value)) { //table column, called for each column cell!!!
      // console.log("changeHTML value array", node.parentNode.parentNode.nodeName.toLocaleLowerCase(), node.id, (rowNr==-1)?JSON.stringify(commandJson.value):commandJson.value[rowNr], commandJson.chk, rowNr);

      if (rowNr == -1) {
        console.log("changeHTML value array should not happen when no rowNr", variable, node, commandJson, rowNr);
        let newRowNr = 0;
        for (let val of commandJson.value) {
          let nodeId = node.id + "#" + newRowNr; //tbd: not variable id? using node.id var#x#y possible for nested tables?
          if (gId(nodeId)) {
            // console.log("changeHTML value array recursive", variable, node.id, gId(nodeId), val);
            changeHTML(variable, gId(nodeId), {"value":val, "chk":"Array1"}, newRowNr); //recursive set value for variable in row
          }
          else
            console.log("changeHTML node not found", nodeId, node, commandJson);
            newRowNr++;
        }
      }
      else {
        changeHTML(variable, node, {"value":commandJson.value[rowNr], "chk":"Array2"}, rowNr); //recursive set value for variable in row
      }
      // node.checked = commandJson.value;
    } 
    else if (nodeType == "span") { //read only vars
      if (node.className == "select") {
        var index = 0;
        if (variable.options && commandJson.value != null) { // not always the case e.g. data / table / uiFun. Then value set if uiFun returns
          for (var value of variable.options) {
            if (parseInt(commandJson.value) == index) {
              // console.log("changeHTML select1", value, node, node.textContent, index);
              node.textContent = value; //replace the id by its value
              // console.log("changeHTML select2", value, node, node.textContent, index);
            }
            index++;
          }
        } else
          node.textContent = commandJson.value;
      }
      else { //text and numbers read only
        // console.log("changeHTML value span not select", variable, node, commandJson, rowNr);
        node.textContent = commandJson.value;
      }
    }
    else if (node.className == "url") { //url links
      node.innerText = "🔍";
      node.setAttribute('href', commandJson.value);
    } 
    else if (node.className == "canvas")
      console.log("not called anymore");
    else if (node.className == "checkbox") {
      node.checked = commandJson.value;
      node.indeterminate = (commandJson.value == null); //set the false if it has a non null value
    }
    else if (node.className == "button") {
      if (commandJson.value) node.value = commandJson.value; //else the id / label is used as button label
    }
    else if (node.className == "coord3D") {
      console.log("chHTML value coord3D", node, commandJson.value, rowNr);

      if (commandJson.value && Object.keys(commandJson.value)) { //tbd: support arrays (now only objects)
        let index = 0;
        for (let key of Object.keys(commandJson.value)) {
          let childNode = node.childNodes[index++];
          childNode.value = commandJson.value[key];
          childNode.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
        }
      }
      else 
        console.log("   value coord3D value not object[x,y,z]", commandJson.value);
    }
    else {//inputs or select
      node.value = commandJson.value;
      node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input

      //'hack' show the serverName on top of the page
      if (variable.id == "serverName")
        gId("instanceName").innerText = commandJson.value;
    }

    //value assignments depending on different situations

    if ((variable.value == null || !Array.isArray(variable.value)) && !Array.isArray(commandJson.value) && rowNr == -1) {
      //no arrays and rowNr. normal situation
      if (variable.value != commandJson.value)
        variable.value = commandJson.value;
    }
    else if ((variable.value == null || Array.isArray(variable.value)) && Array.isArray(commandJson.value)) {
      //both arrays
      if (rowNr) {
        if (variable.value == null) variable.value = [];
        if (variable.value[rowNr] != commandJson.value[rowNr]) {
          variable.value[rowNr] = commandJson.value[rowNr];
      }
      else {
        if (variable.value != commandJson.value)
          variable.value = commandJson.value;
      }
    }
    }
    else if ((variable.value == null || Array.isArray(variable.value)) && !Array.isArray(commandJson.value)) {
      //after changeHTML value array
      if (variable.value == null) variable.value = [];
      if (variable.value[rowNr] != commandJson.value) {
        variable.value[rowNr] = commandJson.value;
      }
    }
    else if (!Array.isArray(variable.value) && !Array.isArray(commandJson.value) && rowNr != -1) {
      if (variable.value != commandJson.value) {
        console.log("chHTML column with one value for all rows", variable.id, node.id, variable.value, commandJson.value, rowNr);
        variable.value = commandJson.value;
      }
    }
    else
      console.log("chHTML value unknown", variable.id, node.id, variable.value, commandJson.value, rowNr);

  } //value

  if (commandJson.hasOwnProperty("json")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML json", variable, node, commandJson, rowNr);
    jsonValues[node.id] = commandJson.json;
    // variable[node.id].json = commandJson.json;
  }

  if (commandJson.hasOwnProperty("file")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML file", variable, node, commandJson, rowNr);
  
    //we need to send a request which the server can handle using request variable
    let url = `http://${window.location.hostname}/file`;
    fetchAndExecute(url, commandJson.file, node.id, function(id, text) { //send node.id as parameter
      // console.log("fetchAndExecute", text); //in case of invalid commandJson
      var ledmapJson = JSON.parse(text);
      jsonValues[id] = ledmapJson;
      jsonValues[id].new = true;
      // variable[id].file = ledmapJson;
      // variable[id].file.new = true;
      console.log("fetchAndExecute", jsonValues);
    }); 
  }
} //changeHTML

function flushUIFunCommands() {
  if (uiFunCommands.length > 0) { //if something to flush
    var command = {};
    command["uiFun"] = uiFunCommands; //ask to run uiFun for vars (to add the options)
    // console.log("flushUIFunCommands", command);
    requestJson(command);
    uiFunCommands = [];
  }
}

function findVar(id, parent = model) {
  // console.log("findVar", id, parent, model);

  let foundVar = null;
  for( var variable of parent) {
    if (foundVar == null) {
      if (variable.id == id)
        foundVar = variable;
      else if (variable.n)
        foundVar = findVar(id, variable.n);
    }
  }
  return foundVar;
}

var jsonTimeout;
var reqsLegal = false;

function requestJson(command) {
  gId('connind').style.backgroundColor = "var(--c-y)";
	if (command && !reqsLegal) return; // stop post requests from chrome onchange event on page restore
	if (!jsonTimeout) jsonTimeout = setTimeout(()=>{if (ws) ws.close(); ws=null; console.log("connection failed")}, 3000);

  // if (!ws) return;
  let req = JSON.stringify(command);
  
  console.log("requestJson", command);

  if (req.length > 1340)
  console.log("too big???");
  
  ws.send(req?req:'{"v":true}');

  return;
  
  let url = `http://${window.location.hostname}/json`;
  //not used at the moment as WebSockets only
  fetch(url, {
    method: 'post',
    headers: {
      "Content-type": "application/json; charset=UTF-8"
    },
    body: req
  })
  .then(res => {
    if (res) console.log("requestJson res", res, res.json());
  })
  .then(json => {
    if (json) console.log("requestJson json", json);
  })
  .catch((e)=>{
    console.log("requestJson catch", e);
  });
}

function sendValue(element) {
  let varId;
  if (element.id == "saveModel" || element.id == "bSave") {
    varId = "saveModel";
    gId("bSave").value = "Save";
    gId("bSave").disabled = true;
  }
  else 
  {
    varId = element.id;
    gId("bSave").value = "Save*";
    gId("bSave").disabled = false;
  }

  var command = {};
  if (element.type == "checkbox")
    command[varId] = element.checked;
  else if (element.nodeName.toLocaleLowerCase() == "span")
    command[varId] = element.innerText;
  else
    command[varId] = Number(element.value)?Number(element.value):element.value; //type number is default but html converts numbers in <option> to string
  // console.log("sendValue", command);

  
  requestJson(command);
}

let isModal = false;
let modalPlaceHolder;

function toggleModal(element) { //canvas or textarea
  // console.log("toggleModal", element);
  isModal = !isModal;

	if (isModal) {

    modalPlaceHolder = cE(element.nodeName.toLocaleLowerCase()); //create canvas or textarea
    modalPlaceHolder.width = element.width;
    modalPlaceHolder.height = element.height;

    element.parentNode.replaceChild(modalPlaceHolder, element); //replace by modalPlaceHolder

    // let btn = cE("button");
    // btn.innerText = "close";
    // btn.addEventListener('click', (event) => {toggleModal(element);});
    // gId('modalView').appendChild(btn);

    gId('modalView').appendChild(element);
    element.width = window.innerWidth;
    element.height = window.innerHeight;
    // console.log("toggleModal +", element, modalPlaceHolder, element.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
	}
  else {    
    element.width = modalPlaceHolder.getBoundingClientRect().width;
    element.height = modalPlaceHolder.getBoundingClientRect().height;

    // console.log("toggleModal -", element, modalPlaceHolder, element.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
    
    modalPlaceHolder.parentNode.replaceChild(element, modalPlaceHolder); // //replace by element. modalPlaceHolder loses rect
  }

	gId('modalView').style.transform = (isModal) ? "translateY(0px)":"translateY(100%)";
}
// https://stackoverflow.com/questions/324303/cut-and-paste-moving-nodes-in-the-dom-with-javascript

function initCap(s) {
  if (typeof s !== 'string') return '';
  // https://www.freecodecamp.org/news/how-to-capitalize-words-in-javascript/
  return s.replace(/[\W_]/g,' ').replace(/(^\w{1})|(\s+\w{1})/g, l=>l.toUpperCase()); // replace - and _ with space, capitalize every 1st letter
}


//drag and drop functionality
//===========================

var dragSrcEl;

// https://stackoverflow.com/questions/75698658/how-can-i-drag-and-drop-like-browser-tabs-in-javascript
function initMdlColumns() {

  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    column.addEventListener('dragover', handleDragOver);
    column.addEventListener('dragenter', handleDragEnter);
    column.addEventListener('dragleave', handleDragLeave);
    column.addEventListener('drop', handleDrop);
  });

  setupModules();
  
}

function setupModules() {
  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    let modules = column.childNodes;
    modules.forEach(function(module) {
      setupModule(module);
    });
  });
}

// var lastPage;
function setupModule(item) {
  item.addEventListener('dragstart', handleDragStart);
  item.addEventListener('dragover', handleDragOver);
  item.addEventListener('dragenter', handleDragEnter);
  item.addEventListener('dragleave', handleDragLeave);
  item.addEventListener('dragend', handleDragEnd);
  item.addEventListener('drop', handleDrop);
  // item.onclick = function() {
  //   console.log("click", this, lastPage);
  //   if (lastPage) document.getElementById(lastPage.id+"-page").hidden = true;
  //   document.getElementById(this.id+"-page").hidden = false;
  //   lastPage = this;
  // };
}

function handleDragStart(e) {
  this.style.opacity = '0.4';

  dragSrcEl = this;

  e.dataTransfer.effectAllowed = 'move';
  e.dataTransfer.setData('text/html', this.innerText);
  console.log("handleDragStart", this, e, e.dataTransfer);
  e.dataTransfer.setData('text/plain', this.id);
}

function removeDragStyle(item) {
  item.style.opacity = '1';

  let columns = gId("mdlContainer").childNodes;
  columns.forEach(function(column) {
    let modules = column.childNodes;
    modules.forEach(function(module) {
      module.classList.remove('over');
    });
    column.classList.remove('over');
  });
}

function handleDragEnd(e) {
  console.log("handleDragEnd", this, e);
  removeDragStyle(this);
}

function handleDragOver(e) {
  e.preventDefault();
  return false;
}

function handleDragEnter(e) {
  this.classList.add('over');
}

function handleDragLeave(e) {
  this.classList.remove('over');
}

function handleDrop(e) {
  e.stopPropagation();

  if (dragSrcEl !== this) {
    console.log("handleDrop", dragSrcEl, this, e, e.dataTransfer);

    const clone = dragSrcEl.cloneNode(true);
    setupModule(clone);
    removeDragStyle(clone);

    if (this.id.includes("mdlColumn")) {
      console.log("coladd");
      this.appendChild(clone);
    } else {
      this.parentNode.insertBefore(clone, this.nextSibling);
    }

    dragSrcEl.remove();
  }

  return false;
}

//WLEDMM: utility function to load contents of file from FS (used in draw)
function fetchAndExecute(url, name, parms, callback, callError = null)
{
  fetch
  (url+name, {
    method: 'get'
  })
  .then(res => {
    if (!res.ok) {
		callError("File " + name + " not found");
    	return "";
    }
    // console.log("res", res, res.text(), res.text().result);
    return res.text();
  })
  .then(text => {
    // console.log("text", text);
    callback(parms, text);
  })
  .catch(function (error) {
	if (callError) callError(parms, "Error getting " + name);
	console.log(error);
  })
  .finally(() => {
    // if (callback) setTimeout(callback,99);
  });
}

function setInstanceTableColumns() {

  let tbl = gId("insTbl");
  if (!tbl) return;
  let isStageView = gId("vStage").classList.contains("selected");
  let thead = tbl.querySelector("thead");
  let tbody = tbl.querySelector("tbody");

  function showHideColumn(colNr, doHide) {
    // console.log("showHideColumn", thead.parentNode.parentNode, colNr, doHide);
    thead.querySelector("tr").childNodes[colNr].hidden = doHide;
    for (let row of tbody.childNodes) {
      // console.log("   row", row, row.childNodes, i);
      if (colNr < row.childNodes.length) //currently there are comments in the table header ...
        row.childNodes[colNr].hidden = doHide;
    }
  }

  // console.log("setInstanceTableColumns", tbl, thead, tbody);
  columnNr = 2;
  for (; columnNr<6; columnNr++) {
    showHideColumn(columnNr, isStageView);
  }
  for (; columnNr<thead.querySelector("tr").childNodes.length; columnNr++) {
    showHideColumn(columnNr, !isStageView);
  }

  gId("sma").parentNode.hidden = isStageView; //hide sync master label field and comment
}

function changeHTMLView(value) {

  // console.log("changeHTMLView", node, node.value, node.id, mdlContainerNode, mdlContainerNode.childNodes);
  
  gId("vApp").classList.remove("selected");
  gId("vStage").classList.remove("selected");
  gId("vUser").classList.remove("selected");
  gId("vSys").classList.remove("selected");
  gId("vAll").classList.remove("selected");
  gId(value).classList.add("selected");

  let mdlContainerNode = gId("mdlContainer"); //class mdlContainer

  let columnCounter = 0;
  for (let mdlColumnNode of mdlContainerNode.childNodes) {
    let mdlFound = false;
    for (let divNode of mdlColumnNode.childNodes) {
      let found = false;
      if (value == "vAll")
        found = true;
      else {
        for (let moduleNode of divNode.childNodes) {
          if (moduleNode.className) {
            if (value=="vApp" && moduleNode.className == "appmod")
              found = true;
              if (value=="vSys" && moduleNode.className == "sysmod")
              found = true;
            if (value=="vUser" && moduleNode.className == "usermod")
              found = true;
            if (value=="vStage" && moduleNode.id == "Instances")
              found = true;
          }
          // console.log(mdlColumnNode, moduleNode, moduleNode.className);
        }
      }
      divNode.hidden = !found;
      if (found) mdlFound = true;
    }

    mdlColumnNode.hidden = !mdlFound;
    if (mdlFound) {
      columnCounter++;
    }
  }

  if (value=="vApp")
    mdlContainerNode.className = "mdlContainer2";
  else
    mdlContainerNode.className = "mdlContainer" + columnCounter; //1..4

  setInstanceTableColumns();

} //changeHTMLView

//https://webdesign.tutsplus.com/color-schemes-with-css-variables-and-javascript--cms-36989t
function changeHTMLTheme(value) {
  localStorage.setItem('theme', value);
  document.documentElement.className = value;
  if (gId("theme-select").value != value)
    gId("theme-select").value = value;
}

function saveModel(node) {
  console.log("saveModel", node);

  sendValue(node);
}

function setView(node) {
  var command = {};
  command["view"] = node.id;
  requestJson(command);
}

function setTheme(node) {
  var command = {};
  command["theme"] = node.value;
  requestJson(command);
}

function getTheme() {
  let value = localStorage.getItem('theme');
  if (value && value != "null") changeHTMLTheme(value);
}