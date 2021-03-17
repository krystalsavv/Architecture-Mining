import { slider, checkbox, selector, number, applyButton } from "./components.js";
import UIManager from '../UIManager.js';

export default Vue.component('group', {
    props: [
        'groups',
        'group'
    ],
    components: { slider, checkbox, selector, number, applyButton },
    template:
    `<div class="card">
        
        <button class=" card-header heading-button heading-button-gray py-2 collapsed" data-toggle="collapse" 
            :data-target="'#' + group.id + '-body'" 
            :id="group.id + '-heading'"> 
                <i class="collapse-icon fas fa-xs fa-chevron-down mr-1"></i>
                {{group.description}} 
        </button>
     
        <div :id="group.id + '-body'" class="collapse" :aria-labelledby="group.id + '-heading'">
            <div class='card-body'>
                <div v-for="data in group.data">
                    <div class="groupElem"
                        :class="{disabledElem: evalCondition(data.conditions.disabled)}"
                        v-show="(data.conditions === undefined || data.conditions.displayed === undefined) ? true : evalCondition(data.conditions.displayed)">
                    
                        <div v-if="data.type == 'slider'">
                        <slider :data="data" :disabled="evalCondition(data.conditions.disabled)" :onChange="onChange"/>
                        </div>

                        <div v-else-if="data.type == 'checkbox'">
                        <checkbox :data="data" :disabled="evalCondition(data.conditions.disabled)" :onChange="onChange"/>
                        </div>

                        <div v-else-if="data.type == 'select'">
                            <selector :data="data" :disabled="evalCondition(data.conditions.disabled)" :onChange="onChangeSelect"/>
                        </div>

                        <div v-else-if="data.type == 'number'">
                            <number :data="data" :disabled="evalCondition(data.conditions.disabled)" :onChange="onChange"/>
                        </div>

                        <div v-else-if="data.type == 'applyButton'">
                            <applyButton :data="data" :group="group" :disabled="evalCondition(data.conditions.disabled)"/>
                        </div>

                        <div v-else-if="data.type == 'span'">
                        <span>{{data.description}}</span>
                        </div>

                        <div v-else-if="data.type == 'subgroup'">
                            <group :groups="groups" :group="data" class="subgroup" /> 
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>`,
    methods: {
        onChange: UIManager.onChange,
        onChangeSelect: UIManager.onChangeSelect,
        evalCondition: UIManager.evalCondition
    }
});