/**
 * (C) Copyright 2024 SECO Mind Srl
 * SPDX-License-Identifier: Apache-2.0
 */

window.addEventListener('DOMContentLoaded', (event) => {
  /* re-inject project version at a custom location */
  let version = document.getElementById('projectnumber').innerText
  let titleTable = document.querySelector('#titlearea table');
  let cell = titleTable.insertRow(1).insertCell(0);
  cell.innerHTML = '<div id="projectversion">' + version + '</div>';
});
