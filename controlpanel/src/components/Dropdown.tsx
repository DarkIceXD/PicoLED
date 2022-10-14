/* @jsxImportSource solid-js */
import { createSignal, For } from "solid-js"

const send = (url, key, value) => {
    fetch(url, {
        method: 'POST',
        body: `${key},${value}`
    })
}

const get = (url, key) => {
    return fetch(url + "/get", {
        method: 'POST',
        body: `${key}`
    })
}

export default function (props) {
    const [value, setValue] = createSignal(0);
    get(props.url, props.key)
        .then((response) => response.text())
        .then((result) => setValue(parseInt(result)))
        .catch((error) => console.error('Error:', error))
    return (
        <>
            <select
                class="form-select bg-white dark:bg-slate-800 shadow overflow-hidden rounded-md"
                onChange={(e) => send(props.url, props.key, e.target.value)}>
                <For each={props.options} fallback={<div>Loading...</div>}>
                    {(item, index) =>
                        <option selected={index() == value()} value={index()}>{item}</option>
                    }
                </For>
            </select>
        </>
    )
}